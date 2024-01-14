// ----------------------------------------
// The MIT License
// Copyright 2017 Inigo Quilez
vec2 msign(vec2 v) {
    return vec2((v.x >= 0.0) ? 1.0 : -1.0,
                (v.y >= 0.0) ? 1.0 : -1.0);
}
uint packSnorm2x8(vec2 v) {
    uvec2 d = uvec2(round(127.5 + v * 127.5));
    return d.x | (d.y << 8u);
}
vec2 unpackSnorm2x8(uint d) {
    return vec2(uvec2(d, d >> 8) & 255) / 127.5 - 1.0;
}
uint octahedral_16(in vec3 nor) {
    nor /= (abs(nor.x) + abs(nor.y) + abs(nor.z));
    nor.xy = (nor.z >= 0.0) ? nor.xy : (1.0 - abs(nor.yx)) * msign(nor.xy);
    return packSnorm2x8(nor.xy);
}
vec3 i_octahedral_16(uint data) {
    vec2 v = unpackSnorm2x8(data);
    vec3 nor = vec3(v, 1.0 - abs(v.x) - abs(v.y));
    float t = max(-nor.z, 0.0);
    nor.x += (nor.x > 0.0) ? -t : t;
    nor.y += (nor.y > 0.0) ? -t : t;
    return nor;
}
uint spheremap_16(in vec3 nor) {
    vec2 v = nor.xy * inversesqrt(2.0 * nor.z + 2.0);
    return packSnorm2x8(v);
}
vec3 i_spheremap_16(uint data) {
    vec2 v = unpackSnorm2x8(data);
    float f = dot(v, v);
    return vec3(2.0 * v * sqrt(1.0 - f), 1.0 - 2.0 * f);
}
// ----------------------------------------

f32vec3 u16_to_nrm(u32 x) {
    return normalize(i_octahedral_16(x));
    // return i_spheremap_16(x);
}
f32vec3 u16_to_nrm_unnormalized(u32 x) {
    return i_octahedral_16(x);
    // return i_spheremap_16(x);
}
u32 nrm_to_u16(f32vec3 nrm) {
    return octahedral_16(nrm);
    // return spheremap_16(nrm);
}