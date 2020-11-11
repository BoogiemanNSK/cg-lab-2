#include "acceleration_structures.h"

//#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

AccelerationStructures::AccelerationStructures(short width, short height) : LightingAndShadows(width, height)
{
}

AccelerationStructures::~AccelerationStructures()
{
}

bool TLAS::AABBTest(const Ray& ray) const
{
	float3 inv_ray_dir = float3(1.0) / ray.direction;
	float3 t0 = (aabb_max - ray.position) * inv_ray_dir;
	float3 t1 = (aabb_min - ray.position) * inv_ray_dir;
	float3 tmin = min(t0, t1);
	float3 tmax = max(t0, t1);
	bool result = maxelem(tmin) <= minelem(tmax);
	return result;
}

void TLAS::AddMesh(const Mesh mesh)
{
	if (meshes.empty()) {
		aabb_min = mesh.aabb_min;
		aabb_max = mesh.aabb_max;
	} 	

	meshes.push_back(mesh);

	aabb_min = min(mesh.aabb_min, aabb_min);
	aabb_max = max(mesh.aabb_max, aabb_max);
}

bool cmp(const Mesh& a, const Mesh& b)
{
	return false;
}

void AccelerationStructures::BuildBVH()
{
	std::sort(meshes.begin(), meshes.end(), cmp);
	auto middle = meshes.begin();
	std::advance(middle, 2);
	std::vector<Mesh> left_half(meshes.begin(), middle);
	std::vector<Mesh> right_half(middle, meshes.end());

	TLAS left;
	for (auto& mesh: left_half) {
		left.AddMesh(mesh);
	}

	TLAS right;
	for (auto& mesh: right_half) {
		right.AddMesh(mesh);
	}

	tlases.push_back(left);
	tlases.push_back(right);
}

int AccelerationStructures::LoadGeometry(std::filesystem::path filename)
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
		Mesh mesh;
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

			MaterialTriangle triangle(vertices[0], vertices[1], vertices[2]);
			tinyobj::material_t material = materials[shapes[s].mesh.material_ids[f]];
			
			triangle.SetEmisive(float3(material.emission));
			triangle.SetAmbient(float3(material.ambient));
			triangle.SetDiffuse(float3(material.diffuse));
			triangle.SetSpecular(float3(material.specular), material.shininess);
			triangle.SetReflectiveness(material.illum == 5);
			triangle.SetReflectivenessAndTransparency(material.illum == 7);
			triangle.SetIor(material.ior);

			mesh.AddTriangle(triangle);
		}
		meshes.push_back(mesh);
	}

	return 0;
}

Payload AccelerationStructures::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
	if (max_raytrace_depth == 0) return Miss(ray);
	
	IntersectableData closest_hit_data(t_max);
	MaterialTriangle closest_object;

	for (auto& tlas: tlases) {
		if (!tlas.AABBTest(ray)) continue;
		for (auto& mesh: tlas.GetMeshes()) {
			if (!mesh.AABBTest(ray)) continue;

			for (auto object: mesh.Triangles()) {
				IntersectableData data = object.Intersect(ray);
				if (data.t > t_min && data.t < closest_hit_data.t) {
					closest_hit_data = data;
					closest_object = object;
				}	
			}
		}
	}

	if (closest_hit_data.t < t_max) {
		return Hit(ray, closest_hit_data, &closest_object, max_raytrace_depth - 1);
	}

	return Miss(ray);
}

float AccelerationStructures::TraceShadowRay(const Ray& ray, const float max_t) const {
	for (auto& tlas: tlases) {
		if (!tlas.AABBTest(ray)) continue;
		for (auto& mesh: tlas.GetMeshes()) {
			if (!mesh.AABBTest(ray)) continue;
			for (auto& object: mesh.Triangles()) {
				IntersectableData data = object.Intersect(ray);
				if (data.t > t_min && data.t < max_t) {
					return data.t;
				}
			}
		}
	}
	return max_t;
}

void Mesh::AddTriangle(const MaterialTriangle triangle)
{
	if (triangles.empty()) 
		aabb_min = aabb_max = triangle.a.position;
	
	triangles.push_back(triangle);

	aabb_min = min(triangle.a.position, aabb_min);
	aabb_min = min(triangle.b.position, aabb_min);
	aabb_min = min(triangle.c.position, aabb_min);

	aabb_max = max(triangle.a.position, aabb_max);
	aabb_max = max(triangle.b.position, aabb_max);
	aabb_max = max(triangle.c.position, aabb_max);
}

bool Mesh::AABBTest(const Ray& ray) const
{
	float3 inv_ray_dir = float3(1.0) / ray.direction;
	float3 t0 = (aabb_max - ray.position) * inv_ray_dir;
	float3 t1 = (aabb_min - ray.position) * inv_ray_dir;
	float3 tmin = min(t0, t1);
	float3 tmax = max(t0, t1);
	bool result = maxelem(tmin) <= minelem(tmax);
	return result;
}
