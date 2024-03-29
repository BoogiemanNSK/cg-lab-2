#pragma once

#include "mt_algorithm.h"
#include <filesystem>

class MaterialTriangle : public Triangle
{
public:
	MaterialTriangle();
	MaterialTriangle(Vertex a, Vertex b, Vertex c) : Triangle(a, b, c) {
		geo_normal = normalize(cross(ba, ca));
		specular_exponent = 1.f;
		ior = 1.0f;
	};
	virtual ~MaterialTriangle() {};

	void SetEmisive(float3 emissive) { emissive_color = emissive; };
	void SetAmbient(float3 ambient) { ambient_color = ambient; };
	void SetDiffuse(float3 diffuse) { diffuse_color = diffuse; };
	void SetSpecular(float3 color, float shininess) { specular_color = color; specular_exponent = shininess; };
	void SetReflectiveness(bool reflective) { reflectiveness = reflective; };
	void SetReflectivenessAndTransparency(bool reflective_and_transparent) { reflectiveness_and_transparency = reflective_and_transparent; };
	void SetIor(float in_ior) { ior = in_ior; };

	float3 GetNormal(float3 barycentric) const;

	float3 geo_normal;

	float3 emissive_color;
	float3 ambient_color;
	float3 diffuse_color;
	float3 specular_color;
	float specular_exponent;
	float ior;

	bool reflectiveness = false;
	bool reflectiveness_and_transparency = false;
};

class Light
{
public:
	Light(float3 position, float3 color) : position(position), color(color) {};
	virtual ~Light() {};

	float3 position;
	float3 color;
};

class LightingAndShadows : public MTAlgorithm
{
public:
	LightingAndShadows(short width, short height);
	virtual ~LightingAndShadows();

	virtual int LoadGeometry(std::filesystem::path filename);

	virtual void AddLight(Light* light);
protected:
	virtual Payload TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const;
	virtual float TraceShadowRay(const Ray& ray, const float max_t) const;
	virtual Payload Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* traingle, const unsigned int max_raytrace_depth) const;

	std::vector<MaterialTriangle*> material_objects;
	std::vector<Light*> lights;
};