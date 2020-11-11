#include "mt_algorithm.h"


Sphere::Sphere(float3 center, float radius) :
	center(center), radius(radius)
{
}

Sphere::~Sphere()
{
}

IntersectableData Sphere::Intersect(const Ray& ray) const
{
	float3 oc = ray.position - center;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0f * dot(ray.direction, oc);
	float c = dot(oc, oc) - radius * radius;
	float d = b * b - 4 * a * c;

	if (d < 0) return IntersectableData(d);
	
	float x0 = (-b - sqrt(d)) / (2.f * a);
	float x1 = (-b + sqrt(d)) / (2.f * a);
	
	float t = std::min(x0, x1);
	if (t < 0) t = std::max(x0, x1);

	return IntersectableData(t);	
}



MTAlgorithm::MTAlgorithm(short width, short height) : RayGenerationApp(width, height)
{
}

MTAlgorithm::~MTAlgorithm()
{
}

int MTAlgorithm::LoadGeometry(std::string filename)
{
	objects.push_back(new Sphere(float3{ 2, 0, -1 }, 0.4f));

	Vertex a(float3{ -0.5f, -0.5f, -1 }, float3{ 0, 0, 1 }, float3(), float3{ 1, 0, 0 });
	Vertex b(float3{ 0.5f, -0.5f, -1 }, float3{ 0, 0, 1 }, float3(), float3{ 1, 0, 0 });
	Vertex c(float3{ 0,  0.5f, -1 }, float3{ 0, 0, 1 }, float3(), float3{ 1, 0, 0 });
	objects.push_back(new Triangle(a, b, c));

	return 0;
}

Payload MTAlgorithm::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
	IntersectableData closest_hit_data(t_max);
	
	for (auto& object: objects) {
		IntersectableData data = object->Intersect(ray);
		if (data.t > t_min && data.t < closest_hit_data.t) {
			closest_hit_data = data;
		}
	}

	if (closest_hit_data.t < t_max) {
		return Hit(ray, closest_hit_data);
	}

	return Miss(ray);
}

Payload MTAlgorithm::Hit(const Ray& ray, const IntersectableData& data) const
{
	Payload payload;
	payload.color = data.baricentric;
	
	return payload;
}

Triangle::Triangle() {
	a = Vertex( float3 {0, 0, 0} );
	b = Vertex( float3 {0, 0, 0} );
	c = Vertex( float3 {0, 0, 0} );
	ba = b.position - a.position;
	ca = c.position - a.position;
}

Triangle::Triangle(Vertex a, Vertex b, Vertex c) :
	a(a), b(b), c(c)
{
	ba = b.position - a.position;
	ca = c.position - a.position;
}

Triangle::~Triangle()
{
}

IntersectableData Triangle::Intersect(const Ray& ray) const
{
	float3 pvec = cross(ray.direction, ca);
	float det = dot(ba, pvec);

	if (det > -1e-8 && det < 1e-8)	
		return IntersectableData(-1);

	float inv_det = 1.f / det;

	float3 tvec = ray.position - a.position;
	float u = dot(tvec, pvec) * inv_det;
	if (u < 0 || u > 1)
		return IntersectableData(-1);

	float3 qvec = cross(tvec, ba);
	float v = dot(ray.direction, qvec) * inv_det;
	if (v < 0 || v > 1 || u + v > 1)
		return IntersectableData(-1);

	float t = dot(ca, qvec) * inv_det;
	return IntersectableData(t, float3{ 1.f - u - v, u, v });
}
