#version 330

precision highp float;	// normal floats, makes no difference on desktop computers
	
#define PI        3.14159265358979323
#define TWO_PI    6.28318530717958648
#define INFINITY  1000000.0
#define EPS 0.001


uniform vec2 resolution;	// resolution of the window
uniform float cameraFov;	// field of view of the camera
uniform vec3 skyColor;		// color of the primitive
uniform mat4 camToWorldMatrix;	// camera to world matrix
uniform vec3 lightDirection;
uniform int maxRecursionDepth;
uniform int numRaysPerPixel;
uniform int frameCount;
uniform vec3 lightColor;
uniform float lightIntensity;
uniform mat4 colorDefficiencyMatrix;

out vec4 outColor;		// computed color of the current pixel

/*

TODO: 
 - implement reflection with roughness
 - implement refraction 
 - implement cumulative color with texture for previous frame
   - the accum color should be reset each time the camera moves
   - the accum color should be multiplied, so each frame accounts for 1 / N of the total color
 - implement collisions with:
   - triangles
   - lenses:
	 - these are defines by two spheres and their intersection
  - implement shadow rays
  - implement global illumination

Precedure for each pixel:
- calculate the starting pos and directon (with jitter)
- shoot a ray D (depth) times (no recursion, only one while-loop)
- for each ray:
  - check for intersection with all objects
  - if hit:
	- calculate the color
	- calculate the reflection
	- calculate the refraction
	- calculate the shadow rays
	- calculate the global illumination
	- calculate the final color
  - if no hit:
	- return the sky color (if set)
	- return skybox color (if set)


*/

struct Ray {
	vec3 startPoint; // world space
	vec3 direction; // world space
};

struct Material {
	vec3 diffuseColor;
	float reflectivity;
	float roughness;
	float n; 
};

struct Sphere {
	vec3 center; // world space
	float radius;
	Material material;
};

// object type enum
#define SPHERE 1
#define LENS 2

// ------------------------------------------------------------
// lens typedefs
// ------------------------------------------------------------
// biconvex lens is a focus lens with two overlaping spheres
#define BICONVEX_LENS 1
// plane-convex lens is a focus lens with one flat surface
#define PLANO_CONVEX_LENS 2
// positive meniscus is a focus lens with two overlaping spheres
#define POSITIVE_MENISCUS 3 
// negative meniscus is a diverging lens with two overlaping spheres
#define NEGATIVE_MENISCUS 4
// plane-concave lens is a diverging lens with one flat surface
#define PLANO_CONCAVE_LENS 5
// biconcave lens is a diverging lens with two disjunct spheres
#define BICONCAVE_LENS 6

struct SphereData {
	float radius;
	vec3 center;
};

struct LensData {
	int lensType; // lens type enum
	SphereData sphere1;
	// for planar lenses, this is the sphere part
	// for biconvex and bioncave lenses this is the first sphere (that is pointing forward)
	// for meniscus lenses this is the front sphere part
	SphereData sphere2;
	// for biconvex and biconcave lenses this is the second sphere (that is pointing backward)
	// for meniscus lenses this is the back sphere part
	// for planar lenses this is used to determine the direction of the lens
	float centerRadius; 
};

struct ObjectData {
	SphereData sphereData;
	LensData lensData;
};

struct Object {
	int type;
	ObjectData data;
	Material material;
};

struct IntersectionResult {
	bool hit;
	vec3 intersectionPoint;
	vec3 normal;
	float distance;
	Object object;
};

// TODO: uniform numObjects and object array

// temp:
const int numObjects = 12;

#define EMPTY_SPHERE_DATA SphereData(0, vec3(0, 0, 0))
#define EMPTY_LENS_DATA LensData(0, EMPTY_SPHERE_DATA, EMPTY_SPHERE_DATA, 0.0f)

Object objects[numObjects] = Object[](
	Object(SPHERE, ObjectData(SphereData(5.5, vec3(-5, 0, 10)), EMPTY_LENS_DATA), Material(vec3(1, 0, 0), 0.0, 0.1, 0.0)),
	Object(SPHERE, ObjectData(SphereData(5, vec3(0, 0, 20)), EMPTY_LENS_DATA), Material(vec3(0, 1, 0), 0.2, 0.1, 0.0)),
	Object(SPHERE, ObjectData(SphereData(3.5, vec3(5, 0, 10)), EMPTY_LENS_DATA), Material(vec3(0, 0, 1), 0.6, 0.1, 0.0)),
	Object(SPHERE, ObjectData(SphereData(5, vec3(0, 0, 20)), EMPTY_LENS_DATA), Material(vec3(1, 1, 1), 0.9, 0.1, 0.0)),
	Object(
		LENS, 
		ObjectData(
			EMPTY_SPHERE_DATA, 
			LensData(	
				NEGATIVE_MENISCUS, 
				SphereData(
					10,
					vec3(0, 0, -7)
				),
				SphereData(
					5, 
					vec3(0, 0, -4)
				), 
				2.0
			)
		), 
		Material(
			vec3(0.4), 
			0.5, 
			0.1, 
			1.516
		)
	),
	Object(
		LENS, 
		ObjectData(
			EMPTY_SPHERE_DATA, 
			LensData(	
				POSITIVE_MENISCUS, 
				SphereData(
					3,
					vec3(8, 0, 0)
				),
				SphereData(
					4, 
					vec3(7.5, 0, 0.5)
				), 
				2.0
			)
		), 
		Material(
			vec3(0.4), 
			0.5, 
			0.1, 
			1.516
		)
	),
	Object(
		LENS, 
		ObjectData(
			EMPTY_SPHERE_DATA, 
			LensData(	
				BICONVEX_LENS, 
				SphereData(
					10,
					vec3(-5, 0, -9)
				),
				SphereData(
					5, 
					vec3(-5, 0, 3)
				), 
				2.0
			)
		), 
		Material(
			vec3(0.4), 
			0.5, 
			0.1, 
			1.516
		)
	),
	Object(
		LENS, 
		ObjectData(
			EMPTY_SPHERE_DATA, 
			LensData(	
				BICONCAVE_LENS, 
				SphereData(
					10,
					vec3(-10, 0, 10.5)
				),
				SphereData(
					5, 
					vec3(-10, 0, -5)
				), 
				2.0
			)
		), 
		Material(
			vec3(0.4), 
			0.5, 
			0.1, 
			1.516
		)
	),
	Object(SPHERE, ObjectData(SphereData(5.5, vec3(-15, 0, 10)), EMPTY_LENS_DATA), Material(vec3(0.7, 0.2, 0), 0.1, 0.1, 0.0)),
	Object(
		LENS, 
		ObjectData(
			EMPTY_SPHERE_DATA, 
			LensData(	
				PLANO_CONVEX_LENS, 
				SphereData(
					3,
					vec3(10, 0, 0)
				),
				SphereData(
					0, 
					vec3(9, 0, 2)
				), 
				2.0
			)
		), 
		Material(
			vec3(0.4), 
			0.5, 
			0.1, 
			1.516
		)
	),
	Object(
		LENS, 
		ObjectData(
			EMPTY_SPHERE_DATA, 
			LensData(	
				PLANO_CONCAVE_LENS, 
				SphereData(
					3,
					vec3(0, 5.5, -3.5)
				),
				SphereData(
					0, 
					vec3(0, 5, 0)
				), 
				2.0
			)
		), 
		Material(
			vec3(0.4), 
			0.5, 
			0.1, 
			1.516
		)
	),
	Object(SPHERE, ObjectData(SphereData(3, vec3(10, 5, 10)), EMPTY_LENS_DATA), Material(vec3(0.1, 0.1, 0.9), 0.0, 0.1, 0.0))
);

uint hash( uint x ) {
	x += ( x << 10u );
	x ^= ( x >>  6u );
	x += ( x <<  3u );
	x ^= ( x >> 11u );
	x += ( x << 15u );
	return x;
}

uint _random_call_counter = 0u;

float random(vec2 coord) {
	float efficientRandomValue = fract(sin(dot(coord.xy,
			vec2(12.9898, 78.233)))
			* 43758.5453123);

	uint permutation = hash(uint(frameCount) * 1024u + _random_call_counter++);

	return fract(permutation / 4294967296.0 + efficientRandomValue);
}

void intersectWithPlane(Ray ray, vec3 point, vec3 normal, inout IntersectionResult intersectionResult) {
	
	 if (dot(ray.direction, normal) == 0) {
		 intersectionResult.hit = false;
		 return;
	 }

	 float t = dot(point - ray.startPoint, normal) / dot(ray.direction, normal);
	 vec3 intersectionPoint = ray.startPoint + ray.direction * t;

	 // check for intersection behind ray startPoint
	 if (t < 0) {
		 intersectionResult.hit = false;
		 return;
	 }

	 intersectionResult.hit = true;
	 intersectionResult.intersectionPoint = intersectionPoint;
	 intersectionResult.normal = normal;
	 intersectionResult.distance = t;
}

void intersectWithSphereInTwoPoints(Ray ray, SphereData sphereData, inout IntersectionResult intersectionResult1, inout IntersectionResult intersectionResult2) {

	
	// implement intersecton test
	vec3 oc = ray.startPoint - sphereData.center;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(oc, ray.direction);
	float c = dot(oc, oc) - sphereData.radius * sphereData.radius;
	float discriminant = b * b - 4 * a * c;
	
	if (discriminant <= 0) {
		intersectionResult1.hit = false;
		intersectionResult2.hit = false;
		return;
	}

	float dist1 = (-b - sqrt(discriminant)) / (2.0 * a);
	float dist2 = (-b + sqrt(discriminant)) / (2.0 * a);

	if (max(dist1, dist2) < 0) {
		intersectionResult1.hit = false;
		intersectionResult2.hit = false;
		return;
	}

	if (dist1 >= 0) {
		vec3 intersectionPoint = ray.startPoint + ray.direction * dist1;
		vec3 normal = normalize(intersectionPoint - sphereData.center);

		intersectionResult1.hit = true;
		intersectionResult1.intersectionPoint = intersectionPoint;
		intersectionResult1.normal = normal;
		intersectionResult1.distance = dist1;
	} else {
		intersectionResult1.hit = false;
	}

	if (dist2 >= 0) {
		vec3 intersectionPoint = ray.startPoint + ray.direction * dist2;
		vec3 normal = normalize(intersectionPoint - sphereData.center);

		intersectionResult2.hit = true;
		intersectionResult2.intersectionPoint = intersectionPoint;
		intersectionResult2.normal = normal;
		intersectionResult2.distance = dist2;
	} else {
		intersectionResult2.hit = false;
	}

}

void intersectWithSphere(Ray ray, SphereData sphereData, inout IntersectionResult intersectionResult) {

	// implement intersecton test
	vec3 oc = ray.startPoint - sphereData.center;
	float a = dot(ray.direction, ray.direction);
	float b = 2.0 * dot(oc, ray.direction);
	float c = dot(oc, oc) - sphereData.radius * sphereData.radius;
	float discriminant = b * b - 4 * a * c;
	
	if (discriminant <= 0) {
		intersectionResult.hit = false;
		return;
	}

	float dist1 = (-b - sqrt(discriminant)) / (2.0 * a);
	float dist2 = (-b + sqrt(discriminant)) / (2.0 * a);

	float dist;

	if (dist1 >= 0 && dist2 >= 0) {
		dist = min(dist1, dist2);
	} else if (dist1 >= 0) {
		dist = dist1;
	} else {
		dist = dist2;
	}

	if (dist <= 0) {
		intersectionResult.hit = false;
		return;
	}

	vec3 intersectionPoint = ray.startPoint + ray.direction * dist;
	vec3 normal = normalize(intersectionPoint - sphereData.center);

	intersectionResult.hit = true;
	intersectionResult.intersectionPoint = intersectionPoint;
	intersectionResult.normal = normal;
	intersectionResult.distance = dist;
}

void intersectWithCylinder(Ray ray, vec3 center, vec3 axisDirection, float radius, float height, inout IntersectionResult intersectionResult) {

	// intersect with the body
	float a = dot(ray.direction - axisDirection * dot(ray.direction, axisDirection), ray.direction - axisDirection * dot(ray.direction, axisDirection));
	float b = 2.0 * dot(ray.direction - axisDirection * dot(ray.direction, axisDirection), ray.startPoint - center - axisDirection * dot(ray.startPoint - center, axisDirection));
	float c = dot(ray.startPoint - center - axisDirection * dot(ray.startPoint - center, axisDirection), ray.startPoint - center - axisDirection * dot(ray.startPoint - center, axisDirection)) - radius * radius;
	float discriminant = b * b - 4 * a * c;

	if (discriminant < 0) {
		intersectionResult.hit = false;
		return;
	}

	float dist1 = (-b - sqrt(discriminant)) / (2.0 * a);
	float dist2 = (-b + sqrt(discriminant)) / (2.0 * a);

	vec3 intersectionPoint1 = ray.startPoint + ray.direction * dist1;
	vec3 intersectionPoint2 = ray.startPoint + ray.direction * dist2;

	bool isHit1Good = dist1 >= 0 && dot(intersectionPoint1 - center, axisDirection) >= 0 && dot(intersectionPoint1 - center, axisDirection) <= height;
	bool isHit2Good = dist2 >= 0 && dot(intersectionPoint2 - center, axisDirection) >= 0 && dot(intersectionPoint2 - center, axisDirection) <= height;

	if (!isHit1Good && !isHit2Good) {
		intersectionResult.hit = false;
		return;
	}

	float dist;

	if (isHit1Good && isHit2Good) {
		dist = min(dist1, dist2);
	} else if (isHit1Good) {
		dist = dist1;
	} else {
		dist = dist2;
	}

	vec3 intersectionPoint = ray.startPoint + ray.direction * dist;
	vec3 normal = normalize(intersectionPoint - center - axisDirection * dot(intersectionPoint - center, axisDirection));

	intersectionResult.hit = true;
	intersectionResult.intersectionPoint = intersectionPoint;
	intersectionResult.normal = normal;
	intersectionResult.distance = dist;

}

void intersectWithSpherePart(Ray ray, SphereData sphereData, vec3 spherePartCenterNormal, float maxAngle /* in radians */, inout IntersectionResult intersectionResult) {

	IntersectionResult sphereIntersectionResult1;
	IntersectionResult sphereIntersectionResult2;
	
	sphereIntersectionResult1.hit = false;
	sphereIntersectionResult2.hit = false;
	
	intersectWithSphereInTwoPoints(ray, sphereData, sphereIntersectionResult1, sphereIntersectionResult2);
	
	bool didSphereHit1 = false;
	bool didSphereHit2 = false;

	if (sphereIntersectionResult1.hit == true) {

		float angle = acos(dot(spherePartCenterNormal, normalize(sphereIntersectionResult1.intersectionPoint - sphereData.center)));

		didSphereHit1 = angle <= maxAngle;
			
		if (didSphereHit1) intersectionResult = sphereIntersectionResult1;
	} 
		
	if (sphereIntersectionResult2.hit == true) {

		float angle = acos(dot(spherePartCenterNormal, normalize(sphereIntersectionResult2.intersectionPoint - sphereData.center)));

		didSphereHit2 = angle <= maxAngle;

		if (didSphereHit2) intersectionResult = sphereIntersectionResult2;
	}

	bool didSphereHit = didSphereHit1 || didSphereHit2;

	if (didSphereHit == false) {
		intersectionResult.hit = false;
		return;
	}

	if (didSphereHit1 && didSphereHit2) {
		if (sphereIntersectionResult1.distance < sphereIntersectionResult2.distance) {
			intersectionResult = sphereIntersectionResult1;
		} else {
			intersectionResult = sphereIntersectionResult2;
		}
	}
}

void intersectWithLens(Ray ray, LensData lensData, inout IntersectionResult intersectionResult) {
	
	int numPrimitivesToIntersect;

	switch (lensData.lensType) {
	case PLANO_CONVEX_LENS:
		numPrimitivesToIntersect = 2;
		break;
	case BICONVEX_LENS:
	case BICONCAVE_LENS:
	case PLANO_CONCAVE_LENS:
	case POSITIVE_MENISCUS:
	case NEGATIVE_MENISCUS:
		numPrimitivesToIntersect = 3;
		break;
	default:
		intersectionResult.hit = false;
		return;
	}

	IntersectionResult intersectionResults[3];
	for (int i = 0; i < numPrimitivesToIntersect; i++) {
		intersectionResults[i].hit = false;
	}

	vec3 sphere1Center = lensData.sphere1.center;
	vec3 sphere2Center = lensData.sphere2.center;
	vec3 lensForward = normalize(sphere2Center - sphere1Center);

	bool isPlanar = lensData.lensType == PLANO_CONVEX_LENS || lensData.lensType == PLANO_CONCAVE_LENS;

	if (isPlanar) {
		
		// sphere intersection

		const int indexOfPlaneIntersectionResult = 0;
		const int indexOfSphereIntersection = 1;
		const int indexOfCylinderIntersection = 2; // only in concave lenses
		
		float maxAngle = asin(lensData.centerRadius / lensData.sphere1.radius);

		intersectWithSpherePart(ray, lensData.sphere1, lensForward, maxAngle, intersectionResults[indexOfSphereIntersection]);

		// plane intersection

		vec3 planeNormal = lensForward;

		float R = lensData.sphere1.radius;
		float r = lensData.centerRadius;

		float distanceFromCenterToPlane = sqrt(R * R - r * r);

		vec3 planePoint = lensData.lensType == PLANO_CONVEX_LENS ? 
			sphere1Center + lensForward * distanceFromCenterToPlane
			: sphere2Center;

		intersectWithPlane(ray, planePoint, planeNormal, intersectionResults[indexOfPlaneIntersectionResult]);

		if (intersectionResults[indexOfPlaneIntersectionResult].hit) {

			vec3 intersectionToCenter = intersectionResults[indexOfPlaneIntersectionResult].intersectionPoint - planePoint;
			float distanceToCenter = length(intersectionToCenter);

			intersectionResults[indexOfPlaneIntersectionResult].hit = distanceToCenter < lensData.centerRadius;
		}

		// cylinder intersection

		if (lensData.lensType == PLANO_CONCAVE_LENS) {
			float height = abs(length(sphere2Center - sphere1Center)) - distanceFromCenterToPlane;
			vec3 cylinderCenter = sphere2Center - lensForward * height;
			intersectWithCylinder(ray, cylinderCenter, lensForward, lensData.centerRadius, height, intersectionResults[indexOfCylinderIntersection]);
		}

	} else {

		bool flipLensForward = lensData.lensType == BICONVEX_LENS || lensData.lensType == POSITIVE_MENISCUS || lensData.lensType == NEGATIVE_MENISCUS;

		if (flipLensForward) {
			lensForward = -lensForward;
		}
	
		const int sphere1IntersectionResultIndex = 0;
		const int sphere2IntersectionResultIndex = 1;
		const int cylinderIntersectionResultIndex = 2;

		bool isSphere1PartForwardFacing = lensData.lensType == BICONVEX_LENS || lensData.lensType == POSITIVE_MENISCUS || lensData.lensType == NEGATIVE_MENISCUS;
		bool isSphere2PartForwardFacing = lensData.lensType == BICONCAVE_LENS || lensData.lensType == POSITIVE_MENISCUS || lensData.lensType == NEGATIVE_MENISCUS;

		vec3 sphere1PartDirection = isSphere1PartForwardFacing ? -lensForward : lensForward;
		vec3 sphere2PartDirection = isSphere2PartForwardFacing ? -lensForward : lensForward;

		float maxAngle1 = asin(lensData.centerRadius / lensData.sphere1.radius);
		float maxAngle2 = asin(lensData.centerRadius / lensData.sphere2.radius);

		intersectWithSpherePart(ray, lensData.sphere1, sphere1PartDirection, maxAngle1, intersectionResults[sphere1IntersectionResultIndex]);
		intersectWithSpherePart(ray, lensData.sphere2, sphere2PartDirection, maxAngle2, intersectionResults[sphere2IntersectionResultIndex]);

		// check for cylinder

		float R1 = lensData.sphere1.radius;
		float R2 = lensData.sphere2.radius;
		float r = lensData.centerRadius;
		float dc = abs(length(sphere2Center - sphere1Center));

		float height;
		if (lensData.lensType == BICONCAVE_LENS) {
			height = dc - sqrt(R1 * R1 - r * r) - sqrt(R2 * R2 - r * r);
		} else if (lensData.lensType == BICONVEX_LENS) {
			height = sqrt(R1 * R1 - r * r) + sqrt(R2 * R2 - r * r) - dc;
		} else if (lensData.lensType == POSITIVE_MENISCUS) {
			height = dc + sqrt(R2 * R2 - r * r) - sqrt(R1 * R1 - r * r);
		} else {
			height = sqrt(R1 * R1 - r * r) - dc - sqrt(R2 * R2 - r * r);
		}

		vec3 cylinderCenter;
		if (lensData.lensType == BICONVEX_LENS) {
			cylinderCenter = sphere2Center + lensForward * (sqrt(R2 * R2 - r * r) - height);
		} else if (lensData.lensType == BICONCAVE_LENS) {
			cylinderCenter = sphere1Center + lensForward * (sqrt(R1 * R1 - r * r));
		} else if (lensData.lensType == POSITIVE_MENISCUS) {
			cylinderCenter = sphere2Center - lensForward * (sqrt(R2 * R2 - r * r));
		} else {
			cylinderCenter = sphere2Center - lensForward * (sqrt(R2 * R2 - r * r) + height);
		}

		intersectWithCylinder(ray, cylinderCenter, lensForward, lensData.centerRadius, height, intersectionResults[cylinderIntersectionResultIndex]);
	}

	intersectionResult.hit = false;
	for (int i = 0; i < numPrimitivesToIntersect; i++) {
		bool overwrite = intersectionResults[i].hit && (intersectionResult.hit == false || intersectionResults[i].distance < intersectionResult.distance);
		
		if (overwrite) {
			intersectionResult = intersectionResults[i];
		}
	}
}

bool intersectWithScene(Ray ray, inout IntersectionResult closestHit, inout float closestObjectDistance) {

	for (int i = 0; i < numObjects; i++) {
		
		IntersectionResult intersectionResult;
		
		switch (objects[i].type) {
		case SPHERE:
			intersectWithSphere(ray, objects[i].data.sphereData, intersectionResult);
			break;
		case LENS:
			intersectWithLens(ray, objects[i].data.lensData, intersectionResult);
			break;
		default:
			continue;
		}

		if (intersectionResult.hit == false) {
			continue;
		}

		intersectionResult.object = objects[i];

		if (closestHit.hit == false || intersectionResult.distance < closestHit.distance) {
			closestHit = intersectionResult;
			closestObjectDistance = intersectionResult.distance;
		}
	}

	return closestHit.hit;
}



vec3 shootRay(Ray ray) {

	vec3 result = vec3(0);

	bool hit = false;
	float remainingEnergy = 1.0;
	bool isInsideObject = false;

	bool isRayBouncing = true;

	for (int depth = 0; depth < maxRecursionDepth; depth++) {

		IntersectionResult closestHit;
		closestHit.hit = false;
		float closestObjectDistance = INFINITY;

		bool didRayHit = intersectWithScene(ray, closestHit, closestObjectDistance);

		if (didRayHit == false) {
			isRayBouncing = false;
			break;
		}
		
		hit = true;

		if (closestHit.object.material.n > 0) {
			// refract
			vec3 normal = closestHit.normal;
			
			if (isInsideObject) {
				normal = -normal;
			}

			if (dot(ray.direction, normal) > 0) {
				normal = -normal;
			}

			float n1 = isInsideObject ? closestHit.object.material.n : 1.0;
			float n2 = isInsideObject ? 1.0 : closestHit.object.material.n;

			float n = n1 / n2;
			float cosI = dot(-ray.direction, normal);
			float sinT2 = n * n * (1.0 - cosI * cosI);

			if (sinT2 > 1.0) {
				// total internal reflection
				ray = Ray(closestHit.intersectionPoint + ray.direction * 0.001, reflect(ray.direction, normal));
				isInsideObject = !isInsideObject;
				remainingEnergy *= 0.8;
				continue;
			}

			vec3 refractedDirection = normalize(n * ray.direction + (n * cosI - sqrt(1.0 - sinT2)) * normal);

			ray = Ray(closestHit.intersectionPoint + refractedDirection * 0.001, refractedDirection);

			vec3 color = closestHit.object.material.diffuseColor;
			float intensity = dot(closestHit.normal, -lightDirection);
			result += color * lightColor * lightIntensity * max(intensity, 0) * remainingEnergy;
			remainingEnergy *= 0.8;

			continue;
		}

		if (closestHit.object.material.reflectivity > 0) {
			// reflect
			vec3 reflectionDirection = normalize(reflect(ray.direction, closestHit.normal));

			vec3 coneTopMiddlePoint = closestHit.intersectionPoint + reflectionDirection;
			float randomAngle = random(gl_FragCoord.xy) * TWO_PI;
			float randomRadius = random(gl_FragCoord.xy) * closestHit.object.material.roughness;
			vec3 randomDirection = vec3(
					cos(randomAngle) * randomRadius, 
					random(gl_FragCoord.xy) * closestHit.object.material.roughness, 
					sin(randomAngle) * randomRadius
			);
				
			// defacto recursion
			ray = Ray(closestHit.intersectionPoint + reflectionDirection * 0.001, normalize(reflectionDirection + randomDirection));

			vec3 color = closestHit.object.material.diffuseColor;
			float intensity = dot(closestHit.normal, -lightDirection);
			result += color * lightColor * lightIntensity * max(intensity, 0) * remainingEnergy;
			remainingEnergy *= closestHit.object.material.reflectivity;
			continue;
		}


		vec3 color = closestHit.object.material.diffuseColor;
		float intensity = max(dot(closestHit.normal, -lightDirection), 0);
		result += color + lightColor * lightIntensity * max(intensity, 0);
		remainingEnergy = 0;

		isRayBouncing = false;
		break; // we have no refraction or reflection, so we stop
	}

	result += remainingEnergy * skyColor;

	return result;
}

void main() {	
	float fovRadians = radians(cameraFov);
	float fovTan = tan(fovRadians / 2);

	float imageAspectRatio = resolution.x / resolution.y; // assuming width > height
	float Px = (2 * ((gl_FragCoord.x + 0.5) / resolution.x) - 1) * fovTan * imageAspectRatio;
	float Py = -(1 - 2 * ((gl_FragCoord.y + 0.5) / resolution.y) * fovTan);
	vec3 rayOrigin = vec3(0, 0, 0);
	vec3 rayDirection = vec3(Px, Py, 1); // note that this just equal to Vec3f(Px, Py, -1);

	vec4 originInWorld =  vec4(rayOrigin, 1) * camToWorldMatrix;
	vec3 lookat = rayDirection + rayOrigin;
	vec4 lookatInWorld = vec4(lookat, 1) * camToWorldMatrix;
	vec4 directionInWorld = lookatInWorld - originInWorld;

	vec3 result = vec3(0, 0, 0);

	for (int i = 0; i < numRaysPerPixel; i++) {

		Ray ray = Ray(originInWorld.xyz, normalize(directionInWorld.xyz));

		vec3 color = shootRay(ray);

		result += color / numRaysPerPixel;
	}

//	outColor = vec4(1, 1, 0, 1);
	outColor = vec4(result, 1) * colorDefficiencyMatrix;
//	outColor = vec4(random(gl_FragCoord.xy), random(gl_FragCoord.xy), random(gl_FragCoord.xy), random(gl_FragCoord.xy));
}