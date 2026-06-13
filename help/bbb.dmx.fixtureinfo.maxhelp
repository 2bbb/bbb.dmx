{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 9,
      "minor": 1,
      "revision": 3
    },
    "classnamespace": "box",
    "rect": [
      100,
      100,
      760,
      528
    ],
    "bglocked": 0,
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
    "description": "Inspect fixture patch/profile metadata.",
    "digest": "Inspect fixture patch/profile metadata.",
    "tags": "dmx, fixture, patch, info",
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
            500,
            20
          ],
          "text": "bbb.dmx.fixtureinfo - Inspect fixture patch/profile metadata."
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
            700,
            40
          ],
          "text": "Inspect loaded patch fixtures, or inspect a profile/mode directly: listparams profile_key mode_key."
        }
      },
      {
        "box": {
          "id": "obj-3",
          "maxclass": "newobj",
          "numinlets": 1,
          "numoutlets": 1,
          "patching_rect": [
            50,
            130,
            260,
            22
          ],
          "text": "bbb.dmx.fixtureinfo",
          "outlettype": [
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
            50,
            190,
            140,
            22
          ],
          "text": "print bbb.dmx.fixtureinfo"
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            50,
            250,
            520,
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
          "id": "obj-6",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            50,
            284,
            520,
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
          "id": "obj-7",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            50,
            318,
            520,
            22
          ],
          "text": "listfixtures",
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
            352,
            520,
            22
          ],
          "text": "fixture spot_01",
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
            50,
            386,
            520,
            22
          ],
          "text": "listparams spot_01",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            50,
            420,
            520,
            22
          ],
          "text": "param spot_01 pan",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-11",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            50,
            488,
            520,
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
          "id": "obj-12",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            50,
            420,
            520,
            22
          ],
          "text": "listparams generic.mover.16bit basic16",
          "outlettype": [
            ""
          ]
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
            454,
            520,
            22
          ],
          "text": "modeparams generic.mover.16bit basic16",
          "outlettype": [
            ""
          ]
        }
      },
      {
        "box": {
          "id": "obj-14",
          "maxclass": "comment",
          "numinlets": 0,
          "numoutlets": 0,
          "patching_rect": [
            50,
            522,
            650,
            20
          ],
          "text": "Detailed docs: docs/bbb.dmx.fixtureinfo.md"
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
            "obj-4",
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
            "obj-3",
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
            "obj-9",
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
            "obj-10",
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
            "obj-12",
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
      }
    ]
  }
}