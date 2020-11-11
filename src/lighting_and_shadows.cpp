#include "lighting_and_shadows.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <algorithm>

LightingAndShadows::LightingAndShadows(short width, short height) : MTAlgorithm(width, height)
{
}

LightingAndShadows::~LightingAndShadows()
{
}

int LightingAndShadows::LoadGeometry(std::filesystem::path filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
		filename.string().c_str(), filename.parent_path().string().c_str(), true);

	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		throw std::runtime_error("Can't parse OBJ file");
	}

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			std::vector<Vertex> vertices;

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3*idx.vertex_index+0];
				tinyobj::real_t vy = attrib.vertices[3*idx.vertex_index+1];
				tinyobj::real_t vz = attrib.vertices[3*idx.vertex_index+2];

				if (idx.normal_index < 0) {
					vertices.push_back(Vertex(float3{vx, vy, vz}));
				} else {
					tinyobj::real_t nx = attrib.vertices[3*idx.normal_index+0];
					tinyobj::real_t ny = attrib.vertices[3*idx.normal_index+1];
					tinyobj::real_t nz = attrib.vertices[3*idx.normal_index+2];
					vertices.push_back(Vertex(float3{vx, vy, vz}, float3{nx, ny, nz}));
				}
				
			}
			index_offset += fv;

			MaterialTriangle* triangle = new MaterialTriangle(vertices[0], vertices[1], vertices[2]);
			tinyobj::material_t material = materials[shapes[s].mesh.material_ids[f]];
			triangle->SetEmisive(float3(material.emission));
			triangle->SetAmbient(float3(material.ambient));
			triangle->SetDiffuse(float3(material.diffuse));
			triangle->SetSpecular(float3(material.specular), material.shininess);
			triangle->SetReflectiveness(material.illum == 5);
			triangle->SetReflectivenessAndTransparency(material.illum == 7);
			triangle->SetIor(material.ior);

			material_objects.push_back(triangle);
		}
	}

	return 0;
}

void LightingAndShadows::AddLight(Light* light)
{
	lights.push_back(light);
}

Payload LightingAndShadows::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
	if (max_raytrace_depth == 0) return Miss(ray);
	
	IntersectableData closest_hit_data(t_max);
	MaterialTriangle closest_object;
	
	for (auto& object: material_objects) {
		IntersectableData data = object->Intersect(ray);
		if (data.t > t_min && data.t < closest_hit_data.t) {
			closest_hit_data = data;
			closest_object = *object;
		}
	}

	if (closest_hit_data.t < t_max) {
		return Hit(ray, closest_hit_data, &closest_object, max_raytrace_depth - 1);
	}

	return Miss(ray);
}

float LightingAndShadows::TraceShadowRay(const Ray& ray, const float max_t) const
{
	for (auto& object: material_objects) {
		IntersectableData data = object->Intersect(ray);
		if (data.t > t_min && data.t < max_t) {
			return data.t;
		}
	}
	return max_t;
}


Payload LightingAndShadows::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const
{
	if (max_raytrace_depth == 0) return Miss(ray);

	float3 x = ray.position + ray.direction * data.t;
	Payload payload;
	payload.color = triangle->emissive_color + 0.1f * triangle->ambient_color;

	float3 normal = triangle->GetNormal(data.baricentric);
	
	for (auto& light: lights) {
		Ray to_light_ray(x, light->position - x);

		// Check for shadows
		float to_light_length = length(light->position - x);
		float t = TraceShadowRay(to_light_ray, to_light_length);
		
		if (fabs(t - to_light_length) < 0.001f) {
			// Diffuse
			payload.color += light->color * triangle->diffuse_color * std::max(0.0f, dot(to_light_ray.direction, normal));

			// Specular
			Ray from_light_ray(light->position, x - light->position);
			float3 specular_direction = from_light_ray.direction - 2.f * dot(from_light_ray.direction, normal) * normal;
			payload.color += light->color * triangle->specular_color * powf(std::max(dot(ray.direction, specular_direction), 0.0f), triangle->specular_exponent);
		}
	}

	return payload;
}

MaterialTriangle::MaterialTriangle() {
}

float3 MaterialTriangle::GetNormal(float3 barycentric) const
{
	if (length(a.normal) == 0.f && length(b.normal) == 0.f && length(c.normal) == 0.f) return geo_normal;
	return barycentric.x * a.normal + barycentric.y * b.normal + barycentric.z * c.normal;
}
