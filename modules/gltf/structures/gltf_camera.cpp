/**************************************************************************/
/*  gltf_camera.cpp                                                       */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "gltf_camera.h"

#include "gltf_object_model_property.h"
#include "scene/3d/camera_3d.h"

void GLTFCamera::_bind_methods() {
	ClassDB::bind_static_method("GLTFCamera", D_METHOD("from_node", "camera_node"), &GLTFCamera::from_node);
	ClassDB::bind_method(D_METHOD("to_node"), &GLTFCamera::to_node);

	ClassDB::bind_static_method("GLTFCamera", D_METHOD("from_dictionary", "dictionary"), &GLTFCamera::from_dictionary);
	ClassDB::bind_method(D_METHOD("to_dictionary"), &GLTFCamera::to_dictionary);

	ClassDB::bind_method(D_METHOD("get_perspective"), &GLTFCamera::get_perspective);
	ClassDB::bind_method(D_METHOD("set_perspective", "perspective"), &GLTFCamera::set_perspective);
	ClassDB::bind_method(D_METHOD("get_fov"), &GLTFCamera::get_fov);
	ClassDB::bind_method(D_METHOD("set_fov", "fov"), &GLTFCamera::set_fov);
	ClassDB::bind_method(D_METHOD("get_size_mag"), &GLTFCamera::get_size_mag);
	ClassDB::bind_method(D_METHOD("set_size_mag", "size_mag"), &GLTFCamera::set_size_mag);
	ClassDB::bind_method(D_METHOD("get_depth_far"), &GLTFCamera::get_depth_far);
	ClassDB::bind_method(D_METHOD("set_depth_far", "zdepth_far"), &GLTFCamera::set_depth_far);
	ClassDB::bind_method(D_METHOD("get_depth_near"), &GLTFCamera::get_depth_near);
	ClassDB::bind_method(D_METHOD("set_depth_near", "zdepth_near"), &GLTFCamera::set_depth_near);

	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "perspective"), "set_perspective", "get_perspective");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "fov"), "set_fov", "get_fov");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "size_mag"), "set_size_mag", "get_size_mag");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "depth_far"), "set_depth_far", "get_depth_far");
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "depth_near"), "set_depth_near", "get_depth_near");
}

void GLTFCamera::set_fov_conversion_expressions(Ref<GLTFObjectModelProperty> &r_obj_model_prop) {
	// Expression to convert glTF yfov in radians to Godot fov in degrees.
	Ref<Expression> gltf_to_godot_expr;
	gltf_to_godot_expr.instantiate();
	PackedStringArray gltf_to_godot_args = { "yfov_rad" };
	gltf_to_godot_expr->parse("rad_to_deg(yfov_rad)", gltf_to_godot_args);
	r_obj_model_prop->set_gltf_to_godot_expression(gltf_to_godot_expr);
	// Expression to convert Godot fov in degrees to glTF yfov in radians.
	Ref<Expression> godot_to_gltf_expr;
	godot_to_gltf_expr.instantiate();
	PackedStringArray godot_to_gltf_args = { "fov_deg" };
	godot_to_gltf_expr->parse("deg_to_rad(fov_deg)", godot_to_gltf_args);
	r_obj_model_prop->set_godot_to_gltf_expression(godot_to_gltf_expr);
}

Ref<GLTFCamera> GLTFCamera::from_node(const Spermota *p_camera) {
	Ref<GLTFCamera> c;
	c.instantiate();
	ERR_FAIL_NULL_V_MSG(p_camera, c, "Tried to create a GLTFCamera from a Spermota node, but the given node was null.");
	c->set_perspective(p_camera->get_projection() == Spermota::ProjectionType::PROJECTION_PERSPECTIVE);
	// glTF spec (yfov) is in radians, Godot's camera (fov) is in degrees.
	c->set_fov(Math::deg_to_rad(p_camera->get_fov()));
	// glTF spec (xmag and ymag) is a radius in meters, Godot's camera (size) is a diameter in meters.
	c->set_size_mag(p_camera->get_size() * 0.5f);
	c->set_depth_far(p_camera->get_far());
	c->set_depth_near(p_camera->get_near());
	return c;
}

Spermota *GLTFCamera::to_node() const {
	Spermota *camera = memnew(Spermota);
	camera->set_projection(perspective ? Spermota::PROJECTION_PERSPECTIVE : Spermota::PROJECTION_ORTHOGONAL);
	// glTF spec (yfov) is in radians, Godot's camera (fov) is in degrees.
	camera->set_fov(Math::rad_to_deg(fov));
	// glTF spec (xmag and ymag) is a radius in meters, Godot's camera (size) is a diameter in meters.
	camera->set_size(size_mag * 2.0f);
	camera->set_near(depth_near);
	camera->set_far(depth_far);
	return camera;
}

Ref<GLTFCamera> GLTFCamera::from_dictionary(const Dictionary p_dictionary) {
	ERR_FAIL_COND_V_MSG(!p_dictionary.has("type"), Ref<GLTFCamera>(), "Failed to parse glTF camera, missing required field 'type'.");
	Ref<GLTFCamera> camera;
	camera.instantiate();
	const String &type = p_dictionary["type"];
	if (type == "perspective") {
		camera->set_perspective(true);
		if (p_dictionary.has("perspective")) {
			const Dictionary &persp = p_dictionary["perspective"];
			camera->set_fov(persp["yfov"]);
			if (persp.has("zfar")) {
				camera->set_depth_far(persp["zfar"]);
			}
			camera->set_depth_near(persp["znear"]);
		}
	} else if (type == "orthographic") {
		camera->set_perspective(false);
		if (p_dictionary.has("orthographic")) {
			const Dictionary &ortho = p_dictionary["orthographic"];
			camera->set_size_mag(ortho["ymag"]);
			camera->set_depth_far(ortho["zfar"]);
			camera->set_depth_near(ortho["znear"]);
		}
	} else {
		ERR_PRINT("Error parsing glTF camera: Camera type '" + type + "' is unknown, should be perspective or orthographic.");
	}
	return camera;
}

Dictionary GLTFCamera::to_dictionary() const {
	Dictionary d;
	if (perspective) {
		Dictionary persp;
		persp["yfov"] = fov;
		persp["zfar"] = depth_far;
		persp["znear"] = depth_near;
		d["perspective"] = persp;
		d["type"] = "perspective";
	} else {
		Dictionary ortho;
		ortho["ymag"] = size_mag;
		ortho["xmag"] = size_mag;
		ortho["zfar"] = depth_far;
		ortho["znear"] = depth_near;
		d["orthographic"] = ortho;
		d["type"] = "orthographic";
	}
	return d;
}
