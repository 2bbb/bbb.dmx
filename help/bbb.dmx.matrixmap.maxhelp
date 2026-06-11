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
            100,
            100,
            840,
            540
        ],
        "bglocked": 1,
        "openrect": [
            0,
            0,
            0,
            0
        ],
        "openinpresentation": 0,
        "default_fontsize": 12,
        "default_fontface": 0,
        "default_fontname": "Arial",
        "gridonopen": 2,
        "gridsize": [
            15,
            15
        ],
        "gridsnaponopen": 0,
        "objectsnaponopen": 1,
        "statusbarvisible": 2,
        "toolbarvisible": 2,
        "lefttoolbarpinned": 0,
        "toptoolbarpinned": 0,
        "righttoolbarpinned": 0,
        "bottomtoolbarpinned": 0,
        "toolbars_unpinned_last_save": 0,
        "tallnewobj": 0,
        "boxanimatetime": 200,
        "enablehscroll": 1,
        "enablevscroll": 1,
        "devicewidth": 0,
        "description": "Sample jit.matrix colors into DMX fixture parameters.",
        "digest": "jit.matrix to DMX fixture mapper",
        "tags": "dmx,jitter,matrix,color,fixture,universe",
        "style": "",
        "subpatcher_template": "",
        "assistshowspatchername": 0,
        "boxes": [
            {
                "box": {
                    "id": "obj-1",
                    "maxclass": "comment",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patching_rect": [
                        50,
                        30,
                        650,
                        20
                    ],
                    "text": "bbb.dmx.matrixmap - Sample jit.matrix colors into fixture parameters across multiple DMX universes."
                }
            },
            {
                "box": {
                    "id": "obj-2",
                    "maxclass": "comment",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patching_rect": [
                        50,
                        60,
                        760,
                        38
                    ],
                    "text": "Load a fixture patch and a matrix map JSON. Send a char or float32 jit_matrix. Values are normalized and expanded through u8/u16/u24 fixture params. Left outlet outputs universe <id> + 512 DMX bytes; right outlet reports status/errors."
                }
            },
            {
                "box": {
                    "id": "obj-3",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 2,
                    "patching_rect": [
                        360,
                        145,
                        360,
                        22
                    ],
                    "text": "bbb.dmx.matrixmap @patch patches/rgb-grid.example.json @map maps/rgb-grid.example.json @universe_mode all",
                    "outlettype": [
                        "",
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-4",
                    "maxclass": "print",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        360,
                        210,
                        180,
                        22
                    ],
                    "text": "print matrixmap_universe"
                }
            },
            {
                "box": {
                    "id": "obj-5",
                    "maxclass": "print",
                    "numinlets": 1,
                    "numoutlets": 0,
                    "patching_rect": [
                        570,
                        210,
                        170,
                        22
                    ],
                    "text": "print matrixmap_status"
                }
            },
            {
                "box": {
                    "id": "obj-6",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        50,
                        120,
                        245,
                        22
                    ],
                    "text": "readpatch patches/rgb-grid.example.json",
                    "outlettype": [
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-7",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        50,
                        150,
                        245,
                        22
                    ],
                    "text": "readmap maps/rgb-grid.example.json",
                    "outlettype": [
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-8",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        50,
                        180,
                        70,
                        22
                    ],
                    "text": "dump",
                    "outlettype": [
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-9",
                    "maxclass": "toggle",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        50,
                        245,
                        24,
                        24
                    ],
                    "outlettype": [
                        "int"
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-10",
                    "maxclass": "newobj",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        90,
                        245,
                        80,
                        22
                    ],
                    "text": "metro 250",
                    "outlettype": [
                        "bang"
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-11",
                    "maxclass": "newobj",
                    "numinlets": 1,
                    "numoutlets": 1,
                    "patching_rect": [
                        90,
                        285,
                        145,
                        22
                    ],
                    "text": "jit.noise 4 float32 16 16",
                    "outlettype": [
                        "jit_matrix"
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-12",
                    "maxclass": "comment",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patching_rect": [
                        50,
                        335,
                        720,
                        40
                    ],
                    "text": "Map schema: grid.fixture_pattern expands fixture ids with printf-style %d, cols/rows split the matrix into normalized cells, params maps profile parameters to r/g/b/a/luma/maxrgb or constant:N. char = byte/255; float32 = clamp 0..1."
                }
            },
            {
                "box": {
                    "id": "obj-13",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        50,
                        390,
                        170,
                        22
                    ],
                    "text": "universe_mode changed",
                    "outlettype": [
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-14",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        235,
                        390,
                        165,
                        22
                    ],
                    "text": "plane_order rgba",
                    "outlettype": [
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-15",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        415,
                        390,
                        120,
                        22
                    ],
                    "text": "invert_y 1",
                    "outlettype": [
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-16",
                    "maxclass": "message",
                    "numinlets": 2,
                    "numoutlets": 1,
                    "patching_rect": [
                        550,
                        390,
                        120,
                        22
                    ],
                    "text": "brightness 0.5",
                    "outlettype": [
                        ""
                    ]
                }
            },
            {
                "box": {
                    "id": "obj-17",
                    "maxclass": "comment",
                    "numinlets": 0,
                    "numoutlets": 0,
                    "patching_rect": [
                        50,
                        455,
                        720,
                        36
                    ],
                    "text": "This object does not send Art-Net. Feed its universe messages into bbb.dmx.merge/fade/safety and then into a sender such as bbb.artnet."
                }
            }
        ],
        "lines": [
            {
                "patchline": {
                    "source": [
                        "obj-6",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-7",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-8",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-13",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-14",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-15",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-16",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-9",
                        0
                    ],
                    "destination": [
                        "obj-10",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-10",
                        0
                    ],
                    "destination": [
                        "obj-11",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-11",
                        0
                    ],
                    "destination": [
                        "obj-3",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-3",
                        0
                    ],
                    "destination": [
                        "obj-4",
                        0
                    ]
                }
            },
            {
                "patchline": {
                    "source": [
                        "obj-3",
                        1
                    ],
                    "destination": [
                        "obj-5",
                        0
                    ]
                }
            }
        ]
    }
}
