# driver-mnt-connext-shapes
This project implements a mount that connects to the DDS shapes topics that are used in interoperability demos. The project uses RTI Connext, and has been verified with version 5.3.0.

## Configuration
The mount can be configured by adding a configuration file to the `config` folder of a project that contains JSON similar to this:

```json
{
    "id": "config/square_mount",
    "type": "driver/mnt/connext/shapes/mount",
    "value": {
        "domainId": 1,
        "topic": "Square",
        "query.from": "data/squares"
    }
}
```
This will listen for the topic `Square` on domain `1`, and mount data under `data/squares`.

## Semantic annotations
The mount uses the `MyShapeType` type (see `model.cx`) type to instantiate shapes in the corto store. This type mirrors the DDS `ShapeType` type, but adds semantic annotations to the members. These unambiguously let applications know what the meaning of a member is, which can then be interpreted by applications that have no built-in knowledge about the shape type.
