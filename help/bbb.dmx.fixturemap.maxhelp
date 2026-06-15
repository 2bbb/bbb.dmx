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
          "text": "bbb.dmx.fixturemap \u2014 map fixture parameters into one or more DMX universes"
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
            52
          ],
          "text": "Load a patch JSON file with read or @patch, then send semantic fixture updates. Default output is a selected 512 integer list; bangall / @universe_mode all outputs universe id plus 512 values for each known universe."
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
            310,
            22
          ],
          "text": "set spot_01 dimmer 255 red 255 green 255 blue 0",
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
            350,
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
          "id": "obj-27",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            510,
            150,
            280,
            22
          ],
          "text": "setall dimmer 255 red 255 green 255 blue 0",
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
            30,
            180,
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
            230,
            180,
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
          "id": "obj-28",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "patching_rect": [
            510,
            180,
            280,
            22
          ],
          "text": "nsetall red 1. green 1. blue 0.",
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
            260,
            260,
            22
          ],
          "text": "bbb.dmx.fixturemap @patch patches/example.json @universe 1",
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
            315,
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
            370,
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
            315,
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
            240,
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
            260,
            260,
            36
          ],
          "text": "Left outlet: selected bare 512-byte list, or universe id + 512 bytes in all mode. Right outlet: status/error messages."
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
            425,
            720,
            45
          ],
          "text": "Patch files reference fixture profile JSON files relative to the patch file. A patch may contain fixtures across multiple universes; use @universe for selected output or @universe_mode all / bangall for explicit multi-universe messages."
        }
      },
      {
        "box": {
          "id": "obj-17",
          "maxclass": "message",
          "patching_rect": [
            350,
            180,
            220,
            22
          ],
          "text": "set spot_01 pan_tilt 32768 32768"
        }
      },
      {
        "box": {
          "id": "obj-18",
          "maxclass": "message",
          "patching_rect": [
            30,
            210,
            190,
            22
          ],
          "text": "channels 1 255 2 128 3 0"
        }
      },
      {
        "box": {
          "id": "obj-19",
          "maxclass": "message",
          "patching_rect": [
            230,
            210,
            55,
            22
          ],
          "text": "dump"
        }
      },
      {
        "box": {
          "id": "obj-20",
          "maxclass": "message",
          "patching_rect": [
            295,
            210,
            55,
            22
          ],
          "text": "reset"
        }
      },
      {
        "box": {
          "id": "obj-21",
          "maxclass": "message",
          "patching_rect": [
            360,
            210,
            50,
            22
          ],
          "text": "clear"
        }
      },
      {
        "box": {
          "id": "obj-22",
          "maxclass": "message",
          "patching_rect": [
            420,
            210,
            85,
            22
          ],
          "text": "autobang 0"
        }
      },
      {
        "box": {
          "id": "obj-23",
          "maxclass": "comment",
          "patching_rect": [
            30,
            485,
            760,
            48
          ],
          "text": "Current API: read/reload/dump/clear/reset/bang/bangall, set/setall/nset/nsetall, color/colorall, shutter/shutterall, track/trackall/trackrel/trackallrel/trackreset, ptbytes, raw channel/channels, @patch, @autobang, @universe_mode, color/tracking attrs."
        }
      },
      {
        "box": {
          "id": "obj-24",
          "maxclass": "comment",
          "patching_rect": [
            30,
            540,
            760,
            48
          ],
          "text": "color/colorall maps semantic RGB onto RGB, RGBW, or CMY fixture profiles. shutter/shutterall opens or closes semantic shutter/strobe channels and overwrites shared strobe channels. track/trackall use loaded fixture position, rotation, calibration, and pan/tilt ranges."
        }
      },
      {
        "box": {
          "id": "obj-25",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            350,
            110,
            60,
            22
          ],
          "text": "bangall"
        }
      },
      {
        "box": {
          "id": "obj-26",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            420,
            110,
            125,
            22
          ],
          "text": "universe_mode all"
        }
      },
      {
        "box": {
          "id": "obj-29",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            240,
            180,
            22
          ],
          "text": "track spot_01 0. 0. 1.5"
        }
      },
      {
        "box": {
          "id": "obj-30",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            270,
            180,
            22
          ],
          "text": "trackall 0. 0. 1.5"
        }
      },
      {
        "box": {
          "id": "obj-31",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            30,
            300,
            145,
            22
          ],
          "text": "trackreset spot_01"
        }
      },
      {
        "box": {
          "id": "obj-32",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            590,
            210,
            190,
            22
          ],
          "text": "color spot_01 rgb 1. 0.8 0."
        }
      },
      {
        "box": {
          "id": "obj-33",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            590,
            240,
            170,
            22
          ],
          "text": "shutter spot_01 1"
        }
      },
      {
        "box": {
          "id": "obj-34",
          "maxclass": "message",
          "numinlets": 2,
          "numoutlets": 1,
          "outlettype": [
            ""
          ],
          "patching_rect": [
            590,
            270,
            105,
            22
          ],
          "text": "colorall rgb8 255 204 0"
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
      },
      {
        "patchline": {
          "source": [
            "obj-17",
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
            "obj-18",
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
            "obj-19",
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
            "obj-20",
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
            "obj-21",
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
            "obj-22",
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
            "obj-25",
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
            "obj-26",
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
            "obj-27",
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
            "obj-28",
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
            "obj-29",
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
            "obj-30",
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
            "obj-31",
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
            "obj-32",
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
            "obj-33",
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
            "obj-34",
            0
          ],
          "destination": [
            "obj-10",
            0
          ]
        }
      }
    ]
  }
}