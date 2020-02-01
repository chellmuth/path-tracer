import json
import os
from pathlib import Path

import fov_converter

MoanaPath = Path(os.environ["MOANA_ROOT"])

def convert_camera(camera_json):
    pathed_json = {
        "name": camera_json["name"],
        "fov": str(fov_converter.fov_x_deg_to_fov_y_deg(camera_json["fov"], 238, 100)),
        "lookAt": {
            "origin": [ str(f) for f in camera_json["eye"] ],
            "target": [ str(f) for f in camera_json["look"] ],
            "up": [ str(f) for f in camera_json["up"] ],
        }
    }

    return pathed_json

def convert_cameras(cameras_directory, out_path):
    sensors_json = []

    for camera_file in cameras_directory.glob("*.json"):
        sensors_json.append(convert_camera(json.load(open(camera_file, "r"))))

    pathed_json = {
        "comment": "Generated by tools/moana.py",
        "sensors": sensors_json
    }

    with open(out_path, "w") as f:
        json.dump(pathed_json, f, indent=2)

def convert_element(element_json, out_path):
    instance_json = {
        "type": "obj",
        "name": element_json["name"],
        "filename": str(MoanaPath / element_json["geomObjFile"])
    }

    instances_json = [
        {
            "type": "instance",
            "name": element_json["name"],
            "transform": [ str(f) for f in element_instance_json["transformMatrix"] ]
        }
        for element_instance_json in element_json.get("instancedCopies", {}).values()
    ]
    instances_json.append(
        {
            "type": "instance",
            "name": element_json["name"],
            "transform": [ str(f) for f in element_json["transformMatrix"] ]
        }
    )

    pathed_json = {
        "instances": [ instance_json ],
        "models": instances_json
    }

    with open(out_path, "w") as f:
        json.dump(pathed_json, f, indent=2)

def convert_elements(elements_directory, out_directory):
    for element_directory in elements_directory.glob("is*"):
        element_path = element_directory / f"{element_directory.name}.json"
        out_path = out_directory / f"{element_directory.name}.json"
        convert_element(json.load(open(element_path, "r")), out_path)


if __name__ == "__main__":
    # convert_cameras(
    #     Path("/home/cjh/moana/island/json/cameras"),
    #     Path("../moana/sensors.json")
    # )

    convert_elements(
        MoanaPath / "json",
        Path("../moana/")
    )
