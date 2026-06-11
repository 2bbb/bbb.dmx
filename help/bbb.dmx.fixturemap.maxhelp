{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 9,
      "minor": 1,
      "revision": 3,
      "architecture": "arm64",
      "modernui": 1
    },
    "classnamespace": "box",
    "rect": [
      100,
      100,
      820,
      470
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
    "description": "Map fixture parameters into a 512-channel DMX universe.",
    "digest": "",
    "tags": "dmx, fixture, universe",
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
            30,
            20,
            620,
            24
          ],
          "text": "bbb.dmx.fixturemap \u2014 map fixture parameters into a 512-channel DMX universe"
        }
      },
      {
        "box": {
          "id": "obj-2",
          "maxclass": "comment",
          "numinlets": 0,
          "numoutlets": 0,
          "patching_rect": [
            30,
            50,
            760,
            38
          ],
          "text": "Load a patch JSON file, then send semantic fixture updates. Output is a 512 integer list: DMX channel 1 first, channel 512 last."
        }
      },
      {
        "box": {
          "id": "obj-3",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            30,
            110,
            190,
            22
          ],
          "text": "read patches/example.json",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-4",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            230,
            110,
            55,
            22
          ],
          "text": "reload",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            295,
            110,
            45,
            22
          ],
          "text": "bang",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            30,
            150,
            150,
            22
          ],
          "text": "set spot_01 dimmer 255",
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
            190,
            150,
            150,
            22
          ],
          "text": "nset spot_01 dimmer 0.5",
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
            350,
            150,
            190,
            22
          ],
          "text": "ptbytes spot_01 127 255 127 255",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-9",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            550,
            150,
            110,
            22
          ],
          "text": "channel 512 255",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 2,
          "patching_rect": [
            250,
            220,
            260,
            22
          ],
          "text": "bbb.dmx.fixturemap @universe 1",
          "outlettype": [
            "list",
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-11",
          "maxclass": "newobj",
          "numinlets": 2,
          "numoutlets": 2,
          "patching_rect": [
            250,
            275,
            85,
            22
          ],
          "text": "zl slice 16",
          "outlettype": [
            "list",
            "list"
          ]
        }
      },
      {
        "box": {
          "id": "obj-12",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            250,
            330,
            145,
            22
          ],
          "text": "print fixturemap-first16"
        }
      },
      {
        "box": {
          "id": "obj-13",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 0,
          "patching_rect": [
            535,
            275,
            135,
            22
          ],
          "text": "print fixturemap-status"
        }
      },
      {
        "box": {
          "id": "obj-14",
          "maxclass": "comment",
          "numinlets": 0,
          "numoutlets": 0,
          "patching_rect": [
            30,
            190,
            170,
            22
          ],
          "text": "Messages go to inlet"
        }
      },
      {
        "box": {
          "id": "obj-15",
          "maxclass": "comment",
          "numinlets": 0,
          "numoutlets": 0,
          "patching_rect": [
            520,
            220,
            260,
            36
          ],
          "text": "Left outlet: full 512-byte universe. Right outlet: status/error messages."
        }
      },
      {
        "box": {
          "id": "obj-16",
          "maxclass": "comment",
          "numinlets": 0,
          "numoutlets": 0,
          "patching_rect": [
            30,
            385,
            720,
            45
          ],
          "text": "Patch files reference fixture profile JSON files relative to the patch file. The included sample patch maps spot_01 at address 1 and spot_02 at address 17 in universe 1."
        }
      }
    ],
    "lines": [
      {
        "patchline": {
          "source": [
            "obj-3",
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
            "obj-4",
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
            "obj-5",
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
            "obj-6",
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
            "obj-7",
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
            "obj-8",
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
            "obj-12",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-10",
            1
          ],
          "destination": [
            "obj-13",
            0
          ]
        }
      }
    ]
  }
}
