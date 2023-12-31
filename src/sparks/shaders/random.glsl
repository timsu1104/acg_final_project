
struct RandomDevice {
  uint seed;
} random_device;

uint WangHash(inout uint seed) {
  seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
  seed *= uint(9);
  seed = seed ^ (seed >> 4);
  seed *= uint(0x27d4eb2d);
  seed = seed ^ (seed >> 15);
  return seed;
}

uint WangHashS(uint seed) {
  seed = uint(seed ^ uint(61)) ^ uint(seed >> uint(16));
  seed *= uint(9);
  seed = seed ^ (seed >> 4);
  seed *= uint(0x27d4eb2d);
  seed = seed ^ (seed >> 15);
  return seed;
}

void InitRandomSeed(uint x, uint y, uint s) {
  random_device.seed = WangHashS(WangHashS(WangHashS(x) ^ y) ^ s);
}

float RandomFloat() {
  float ret = float(WangHash(random_device.seed)) / 4294967296.0;
  random_device.seed ^= uint(61);
  return ret;
}

uint RandomInt(uint bound) {
  uint ret = WangHash(random_device.seed) % bound;
  random_device.seed ^= uint(61);
  return ret;
}

vec2 RandomOnCircle() {
  float theta = RandomFloat() * PI * 2.0;
  return vec2(sin(theta), cos(theta));
}

vec2 RandomInCircle() {
  return RandomOnCircle() * sqrt(RandomFloat());
}

vec3 RandomOnSphere() {
  float theta = RandomFloat() * PI * 2.0;
  float z = RandomFloat() * 2.0 - 1.0;
  float xy = sqrt(1.0 - z * z);
  return vec3(xy * RandomOnCircle(), z);
}

vec3 RandomInSphere() {
  return RandomOnSphere() * pow(RandomFloat(), 0.3333333333333333333);
}

vec3 HenyeyGreenstein(vec3 direction, float g) {
  float cosTheta;
  float u0 = RandomFloat(), u1 = RandomFloat();
  if (abs(g) < 1e-3)
      cosTheta = 1.0 - 2.0 * u0;
  else {
      float sqrTerm = (1.0 - g * g) / (1.0 - g + 2.0 * g * u0);
      cosTheta = (1.0 + g * g - sqrTerm * sqrTerm) / (2.0 * g);
  }
  float sinTheta = sqrt(max(0.0, 1.0 - cosTheta * cosTheta));
  float phi = 2.0 * PI * u1;
  
  vec3 v1, v2;
  if (abs(direction.x) > abs(direction.y))
    v1 = vec3(-direction.z, 0, direction.x) / sqrt(direction.x * direction.x + direction.z * direction.z);
  else
    v1 = vec3(0, direction.z, -direction.y) / sqrt(direction.y * direction.y + direction.z * direction.z);
  v2 = cross(direction, v1);

  return sinTheta * cos(phi) * v1 + sinTheta * sin(phi) * v2 + cosTheta * direction;
}