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
                    "text": "bbb.dmx.mask - Mute, hold, allow, or force raw ranges, fixtures, or groups."
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
                    "text": "Input universe <id> + 512 bytes. Use @patch/@group/@semantic_overrides for fixture/group semantic masks."
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
                    "text": "bbb.dmx.mask @setup setups/show.json @config masks/example.json @patch patch-from-mvr.json @group group.json @semantic_overrides semantic_overrides.json"
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
                    "text": "readsetup setups/show.json / read masks/example.json / readpatch patch-from-mvr.json / readgroups group.json / readoverrides semantic_overrides.json"
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
                    "text": "hold 1 1 16   (raw: universe start count)"
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
                    "text": "mute fixture_id dimmer shutter color"
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
                    "text": "forcefixture fixture_id dimmer 255"
                }
            },
            {
                "box": {
                    "id": "obj-10",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        20.0,
                        326.0,
                        740.0,
                        22.0
                    ],
                    "text": "mutegroup group_id dimmer shutter color"
                }
            },
            {
                "box": {
                    "id": "obj-11",
                    "maxclass": "comment",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        20.0,
                        360.0,
                        740.0,
                        22.0
                    ],
                    "text": "forcegroup group_id dimmer 255 / bangall"
                }
            }
        ],
        "lines": []
    }
}
