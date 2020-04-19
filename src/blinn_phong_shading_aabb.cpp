#include "blinn_phong_shading_aabb.h"
#include <cmath>
#include "first_hit_aabb.h"
#include "globals.h"

// Ambient light default level and reflection displacement constant
#define AMBIENT_LIGHT_DEFAULT_VAL 0.1
#define BP_EPSILON 1e-6

Eigen::Vector3d blinn_phong_shading_aabb(
    const Ray &ray,
    const std::shared_ptr<Object> &descendant,
    const double &t,
    const Eigen::Vector3d &n,
    const std::shared_ptr<AABBTree> &root,
    const std::vector<std::shared_ptr<Light>> &lights)
{
    // set ambient light (fullbright if this surface is a light "fixture" surface).
    double ambient_ia;
    if (descendant->material->is_light)
        ambient_ia = 1.0;
    else
        ambient_ia = AMBIENT_LIGHT_DEFAULT_VAL;
    
    // Set up resources for computing shading from light sources. Shading starts with adding the ambient term.
    // (Ambient term is ignored, set to fullbright (1.0) when showing bounding boxes.)
    Eigen::Vector3d blinn_phong = descendant->material->ka * (G_show_boxes ? 1.0 : ambient_ia);

    Eigen::Vector3d l, dummy_normal;
    Eigen::Vector3d q = ray.origin + t * ray.direction; // Query point 'q'
    Ray ray_to_light;
    ray_to_light.origin = q;
    double t_to_light, t_to_object;
    std::shared_ptr<Object> dummy_descendant;

    // Iterate through lights for this object.
    for (int i = 0; i < lights.size(); i++)
    {
        // Put the direction toward the light in the (normalized) vector l.
        lights[i]->direction(q, l, t_to_light);

        // Give ray to light the direction to THIS light.
        ray_to_light.direction = l;

        // TODO: make a first_hit reduced function specifically for this call.
        bool hit_object = first_hit_aabb(ray_to_light, BP_EPSILON, root, dummy_descendant, t_to_object, dummy_normal);

        // Check if there was no hit, meaning no blocked path to the light.
        // Also check if there was a hit, but that hit was further away than the light.
        // In either case the light is able to reach this point.
        if (!hit_object || (hit_object && t_to_light < t_to_object))
        {
            // Set Lambertian diffuse rgb vector.
            Eigen::Vector3d lambertian = (descendant->material->kd.cwiseProduct(lights[i]->I)) * std::max(0.0, n.dot(l));

            // Construct h, the half-vector, and set specular rgb vector.
            Eigen::Vector3d v, h, specular;
            v = (-1 * ray.direction).normalized();
            h = (v + l).normalized();
            double n_dot_h = n.dot(h);
            if (n_dot_h <= descendant->material->tau) // specular term is negligible
                specular = Eigen::Vector3d(0.0, 0.0, 0.0);
            else
                specular = (descendant->material->ks.cwiseProduct(lights[i]->I)) * pow(std::max(0.0, n_dot_h), descendant->material->phong_exponent);

            // Sum Blinn-Phong rgb values.
            blinn_phong = blinn_phong + lambertian + specular;
        }
    }

    return blinn_phong;
}