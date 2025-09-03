#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "cJSON.h"

#define TO_RADIANS(d) (d * (M_PI / 180))

typedef struct {
  float x, y, z;
} Vector3;

Vector3 addV3(Vector3 a, Vector3 b) {
  return (Vector3) { a.x + b.x, a.y + b.y, a.z + b.z};  
}

Vector3 subV3(Vector3 a, Vector3 b) {
  return (Vector3) { a.x - b.x, a.y - b.y, a.z - b.z};
}

Vector3 mulV3(Vector3 a, float s) {
  return (Vector3) {a.x * s, a.y * s, a.z * s};
}

float magnitudeV3(Vector3 a) {
  return sqrtf((a.x * a.x) + (a.y * a.y) + (a.z * a.z));
}

Vector3 normalizeV3(Vector3 a) {
  float m = magnitudeV3(a);
  return (Vector3) { a.x / m, a.y / m, a.z / m };
}

float dotV3(Vector3 a, Vector3 b) {
  return ((a.x * b.x) + (a.y * b.y) + (a.z * b.z));
}

Vector3 negateV3(Vector3 a) {
  return (Vector3) {-a.x, -a.y, -a.z};
}

typedef float Matrix4x4[4][4];

Vector3 mulV3M4(Matrix4x4 m, Vector3 v) {
  return (Vector3) {
    ((m[0][0] * v.x) + (m[0][1] * v.y) + (m[0][2] * v.z)),
    ((m[1][0] * v.x) + (m[1][1] * v.y) + (m[1][2] * v.z)),
    ((m[2][0] * v.x) + (m[2][1] * v.y) + (m[2][2] * v.z)),
  };
}

typedef struct {
  unsigned char r, g, b;
} Color;

typedef struct {
  Vector3 o;
  float r;
  Color c;
} Sphere;

typedef struct {
  Vector3 o;
  Vector3 d;
} Ray;

typedef struct {
  Vector3 p;
  Vector3 n;
  float d;
  Color c;
  bool hit;
} HitInfo;

typedef struct {
  Vector3 o;
  float theta;
  float fov;
} Camera;

typedef struct {
  unsigned int width, height;
  Color *data;
} Image;

typedef struct {
  unsigned int imgWidth, imgHeight;
  unsigned int sphereCount;
  Sphere *spheres;
  Camera cam;
} SceneInfo;

void writePPMFile(const char *filepath, Image *img) {
  FILE *file = fopen(filepath, "wb");
  if (!file) {
    fprintf(stderr, "ERROR: Failed to open file: %s\n", filepath);
    return;
  }

  fprintf(file, "%s\n%d %d\n%d\n", "P6", img->width, img->height, 255);
  size_t len = img->width * img->height * sizeof(Color);
  fwrite(img->data, len, 1, file);

  fclose(file);
}

HitInfo raySphereIntersect(Ray r, Sphere s) {
  Vector3 oc = subV3(r.o, s.o);
  float a = dotV3(r.d, r.d);
  float b = 2.0f * dotV3(r.d, oc);
  float c = dotV3(oc, oc) - (s.r * s.r);
  float discriminant = (b * b) - (4.0f * a * c);

  float t;
  if (discriminant < 0.0f) {
    return (HitInfo) {0};
  } else if (discriminant == 0.0f) {
    t = (-b - sqrtf(discriminant)) / (2.0f * a);
  } else {
    float t1 = (-b - sqrtf(discriminant)) / (2.0f * a);
    float t2 = (-b + sqrtf(discriminant)) / (2.0f * a);
    t = t1 < t2 ? t1 : t2;
  }

  HitInfo info = {0};
  info.hit = true;
  info.p = addV3(r.o, mulV3(r.d, t));
  info.n = subV3(info.p, s.o);
  info.c = s.c;
  info.d = t;
  return info;
}

void renderImage(Image *img, Camera *cam, Sphere *spheres, size_t sphereCount) {
  float imageAspectRatio = (float)img->width / (float)img->height;

  for (size_t i = 0; i < (img->width * img->height); i++) {
    int x = i % img->width;
    int y = i / img->width;

    float pX = (2.0f * (x + 0.5f) / (float)img->width - 1.0f) * tanf(TO_RADIANS(cam->fov / 2.0f)) * imageAspectRatio;
    float pY = (1.0f - 2.0f * (y + 0.5f) / (float)img->height) * tanf(TO_RADIANS(cam->fov / 2.0f));

    Matrix4x4 camToWorld = (Matrix4x4) {
      {1.0f, 0.0f, 0.0f, 0.0f},
      {0.0f, 1.0f, 0.0f, 0.0f},
      {0.0f, 0.0f, 1.0f, 0.0f},
      {0.0f, 0.0f, 0.0f, 1.0f}
    };

    Vector3 rP = mulV3M4(camToWorld, (Vector3){pX, pY, -1.0f});

    Ray ray = {
      .o = cam->o,
      .d = subV3(rP, cam->o)
    };

    HitInfo closestHitInfo = {.d = 1000.0f};
    for (unsigned int i = 0; i < sphereCount; i++) {
      HitInfo info = raySphereIntersect(ray, spheres[i]);
      if (info.hit) {
        if (info.d < closestHitInfo.d) {
          closestHitInfo = info;
        }
      }
    }

    if (closestHitInfo.hit) {
      img->data[i] = closestHitInfo.c;
      continue;
    }

    // output_value = (input_value - input_min) * (output_max - output_min) / (input_max - input_min) + output_min;
    // unsigned char r = ray.d.x < 0.0f ? 0 : (unsigned char)((ray.d.x * 255.0f) / 1.0f);
    // unsigned char g = ray.d.y < 0.0f ? 0 : (unsigned char)((ray.d.y * 255.0f) / 1.0f);
    // unsigned char b = ray.d.z < 0.0f ? 0 : (unsigned char)((ray.d.z * 255.0f) / 1.0f);

    Color c = {0,0,0};

    img->data[i] = c;
  }
}

char *loadFileContents(const char *filepath, size_t *size) {
  FILE *file = fopen(filepath, "rb");
  if (file == NULL) {
    fprintf(stderr, "ERROR: Couldn't open %s\n", filepath);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t len = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *p = malloc(len + 1);
  fread(p, 1, len, file);
  p[len] = 0;
  *size = len;
  return p;
}

bool readSceneJSON(const char *filepath, SceneInfo *info) {
  size_t jsonSize = 0;
  char *jsonContent = loadFileContents(filepath, &jsonSize);
  if (!jsonContent) {
    return false;
  }

  cJSON *sceneJSON = cJSON_Parse(jsonContent);
  if (!sceneJSON) {
    const char *error = cJSON_GetErrorPtr();
    if (error) {
      fprintf(stderr, "Error before: %s\n", error);
      return false;
    }
  }

  cJSON *camJSON = cJSON_GetObjectItem(sceneJSON, "camera");
  if (cJSON_IsObject(camJSON)) {
    cJSON *fovJSON = cJSON_GetObjectItem(camJSON, "fov");
    if (cJSON_IsNumber(fovJSON)) {
      info->cam.fov = (float)cJSON_GetNumberValue(fovJSON);
    } else {
      info->cam.fov = 70.0f;
    }

    cJSON *rotJSON = cJSON_GetObjectItem(camJSON, "rotation");
    if (cJSON_IsNumber(rotJSON)) {
      info->cam.theta = TO_RADIANS((float)cJSON_GetNumberValue(rotJSON));
    } else {
      info->cam.theta = 0.0f;
    }

    cJSON *originJSON = cJSON_GetObjectItem(camJSON, "origin");
    if (cJSON_IsArray(originJSON)) {
      int size = cJSON_GetArraySize(originJSON);
      if (size >= 3) {
        float *origin = (float *)&info->cam.o;
        for (int i = 0; i < 3; i++) {
          cJSON *element = cJSON_GetArrayItem(originJSON, i);
          if (cJSON_IsNumber(element))
            origin[i] = (float)cJSON_GetNumberValue(element);
          else
            origin[i] = 0.0f;
        }
      }
    }

    cJSON *imgJSON = cJSON_GetObjectItem(sceneJSON, "image");
    if (cJSON_IsObject(imgJSON)) {
      cJSON *widthJSON = cJSON_GetObjectItem(imgJSON, "width");
      if (cJSON_IsNumber(widthJSON)) {
        info->imgWidth = (unsigned int)cJSON_GetNumberValue(widthJSON);
      } else {
        fprintf(stderr, "WARNING: image width were not configured.\nSetting image width to 128.\n");
        info->imgWidth = 128;
      }

      cJSON *heightJSON = cJSON_GetObjectItem(imgJSON, "height");
      if (cJSON_IsNumber(heightJSON)) {
        info->imgHeight = (unsigned int)cJSON_GetNumberValue(heightJSON);
      } else {
        fprintf(stderr, "WARNING: image height were not configured.\nSetting image height to 128.\n");
        info->imgHeight = 128;
      }
    } else {
      fprintf(stderr, "WARNING: image width and height were not configured.\nSetting image width and height to 128.\n");
      info->imgWidth = info->imgHeight = 128;
    }

    cJSON *sphereArrJSON = cJSON_GetObjectItem(sceneJSON, "spheres");
    if (cJSON_IsArray(sphereArrJSON)) {
      int size = cJSON_GetArraySize(sphereArrJSON);
      info->sphereCount = size;
      info->spheres = (Sphere *)malloc(size * sizeof(Sphere));
      for (int i = 0; i < size; i++) {
        cJSON *sphereObjJSON = cJSON_GetArrayItem(sphereArrJSON, i);
        if (cJSON_IsObject(sphereObjJSON)) {
          cJSON *originJSON = cJSON_GetObjectItem(sphereObjJSON, "origin");
          float *origin = (float *)&info->spheres[i].o;
          if (cJSON_IsArray(originJSON)) {
            int size = cJSON_GetArraySize(originJSON);
            if (size >= 3) {
              for (int j = 0; j < 3; j++) {
                cJSON *element = cJSON_GetArrayItem(originJSON, j);
                if (cJSON_IsNumber(element))
                  origin[j] = (float)cJSON_GetNumberValue(element);
                else
                  origin[j] = 0.0f;
              }
            }
          } else {
            for (int j = 0; j < 3; j++) {
              origin[j] = 0.0f;
            }
          }
          
          cJSON *raduisJSON = cJSON_GetObjectItem(sphereObjJSON, "radius");
          if (cJSON_IsNumber(raduisJSON)) {
            info->spheres[i].r = (float)cJSON_GetNumberValue(raduisJSON);
          } else {
            info->spheres[i].r = 0.5f;
          }

          cJSON *colorJSON = cJSON_GetObjectItem(sphereObjJSON, "color");
          unsigned char *color = (unsigned char *)&info->spheres[i].c;
          if (cJSON_IsArray(colorJSON)) {
            int size = cJSON_GetArraySize(colorJSON);
            if (size >= 3) {
              for (int j = 0; j < 3; j++) {
                cJSON *element = cJSON_GetArrayItem(colorJSON, j);
                if (cJSON_IsNumber(element))
                  color[j] = (unsigned char)cJSON_GetNumberValue(element);
                else
                  color[j] = 0;
              }
            }
          } else {
            for (int j = 0; j < 3; j++) {
              color[j] = 0;
            }
          }
        }
      }
    }
  }

  cJSON_Delete(sceneJSON);
  free(jsonContent);
  return true;
}

void printSceneInfo(SceneInfo *info) {
  printf("Scene info:\n");
  printf("  Image info:\n    Width: %d\n    Height: %d\n", info->imgWidth, info->imgHeight);
  printf("  Camera info:\n    Origin: [%.2f, %.2f, %.2f]\n", info->cam.o.x, info->cam.o.y, info->cam.o.z);
  printf("    Rotation: %.2f\n    FOV: %.2f\n", info->cam.theta, info->cam.fov);
  printf("  Spheres info:\n");
  for (unsigned int i = 0; i < info->sphereCount; i++) {
    printf("    Origin: [%.2f, %.2f, %.2f]\n", info->spheres[i].o.x, info->spheres[i].o.y, info->spheres[i].o.z);
    printf("    Radius %.2f\n", info->spheres[i].r);
    printf("    Color: [%d, %d, %d]\n", info->spheres[i].c.r, info->spheres[i].c.g, info->spheres[i].c.b);
  }
}

int main(int argc, const char **argv) {
  const char *sceneFilepath = NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp("--scene", argv[i]) == 0) {
      if (i + 1 > argc) {
        break;
      }
      
      sceneFilepath = argv[i + 1];
      break;
    }
  }

  if (sceneFilepath == NULL) {
    fprintf(stderr, "ERROR: the '--scene' argument was not set.\n");
    return 1;
  }

  SceneInfo sceneInfo = {0};

  if (!readSceneJSON(sceneFilepath, &sceneInfo)) {
    fprintf(stderr, "Failed to read %s\n", sceneFilepath);
    return 1;
  }

  printSceneInfo(&sceneInfo);

  Image img = {
    .width = sceneInfo.imgWidth,
    .height = sceneInfo.imgHeight,
    .data = malloc(img.width * img.height * sizeof(Color))
  };

  renderImage(&img, &sceneInfo.cam, sceneInfo.spheres, sceneInfo.sphereCount);

  writePPMFile("output.ppm", &img);

  free(img.data);
  free(sceneInfo.spheres);

  return 0;
}