{
    "patcher": {
        "fileversion": 1,
        "appversion": {
            "major": 9,
            "minor": 1,
            "revision": 3,
            "architecture": "x64",
            "modernui": 1
        },
        "classnamespace": "box",
        "rect": [
            100.0,
            100.0,
            800.0,
            520.0
        ],
        "boxes": [
            {
                "box": {
                    "id": "obj-1",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        20.0,
                        20.0,
                        740.0,
                        22.0
                    ],
                    "text": "bbb.dmx.curve - Apply curves to multi-universe DMX frames."
                }
            },
            {
                "box": {
                    "id": "obj-2",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        20.0,
                        54.0,
                        740.0,
                        22.0
                    ],
                    "text": "Input universe <id> + 512 bytes. Rules can apply gamma, invert, threshold, or point curves to channel ranges."
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        20.0,
                        88.0,
                        740.0,
                        22.0
                    ],
                    "text": "bbb.dmx.curve @config curves/example.json"
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        20.0,
                        122.0,
                        740.0,
                        22.0
                    ],
                    "text": "print out"
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        20.0,
                        156.0,
                        740.0,
                        22.0
                    ],
                    "text": "print status"
                }
            },
            {
                "box": {
                    "id": "obj-6",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        20.0,
                        190.0,
                        740.0,
                        22.0
                    ],
                    "text": "read curves/example.json"
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        20.0,
                        224.0,
                        740.0,
                        22.0
                    ],
                    "text": "gamma 1 1 512 2.2"
                }
            },
            {
                "box": {
                    "id": "obj-8",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        20.0,
                        258.0,
                        740.0,
                        22.0
                    ],
                    "text": "universe 1 <512 values>"
                }
            },
            {
                "box": {
                    "id": "obj-9",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        20.0,
                        292.0,
                        740.0,
                        22.0
                    ],
                    "text": "bangall"
                }
            }
        ],
        "lines": []
    }
}
