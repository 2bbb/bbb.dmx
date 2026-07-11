{
  "patcher": {
    "fileversion": 1,
    "appversion": {
      "major": 8,
      "minor": 6,
      "revision": 4
    },
    "classnamespace": "box",
    "rect": [
      100.0,
      100.0,
      850.0,
      520.0
    ],
    "bglocked": 1,
    "openinpresentation": 0,
    "default_fontsize": 12.0,
    "default_fontface": 0,
    "default_fontname": "Arial",
    "gridonopen": 2,
    "gridsize": [
      15.0,
      15.0
    ],
    "gridsnaponopen": 0,
    "objectsnaponopen": 1,
    "statusbarvisible": 2,
    "toolbarvisible": 2,
    "description": "Monitor multi-universe DMX data.",
    "digest": "DMX universe monitor",
    "tags": "dmx, lighting, monitor, universe",
    "style": "",
    "boxes": [
      {
        "box": {
          "id": "obj-1",
          "maxclass": "comment",
          "patching_rect": [
            30,
            20,
            620,
            24
          ],
          "text": "bbb.dmx.monitor — inspect multi-universe DMX frames"
        }
      },
      {
        "box": {
          "id": "obj-2",
          "maxclass": "comment",
          "patching_rect": [
            30,
            50,
            720,
            22
          ],
          "text": "Common input: universe <id> <512 values>. Bare 512-value list uses @default_universe."
        }
      },
      {
        "box": {
          "id": "obj-3",
          "maxclass": "message",
          "patching_rect": [
            30,
            95,
            110,
            22
          ],
          "text": "channel 1 1 255"
        }
      },
      {
        "box": {
          "id": "obj-4",
          "maxclass": "message",
          "patching_rect": [
            150,
            95,
            210,
            22
          ],
          "text": "channels 1 1 255 2 128 3 0"
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "message",
          "patching_rect": [
            370,
            95,
            110,
            22
          ],
          "text": "changed_only 1"
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "message",
          "patching_rect": [
            490,
            95,
            45,
            22
          ],
          "text": "bang"
        }
      },
      {
        "box": {
          "id": "obj-7",
          "maxclass": "message",
          "patching_rect": [
            545,
            95,
            60,
            22
          ],
          "text": "bangall"
        }
      },
      {
        "box": {
          "id": "obj-8",
          "maxclass": "message",
          "patching_rect": [
            615,
            95,
            50,
            22
          ],
          "text": "dump"
        }
      },
      {
        "box": {
          "id": "obj-9",
          "maxclass": "newobj",
          "patching_rect": [
            30,
            155,
            220,
            22
          ],
          "text": "bbb.dmx.monitor @default_universe 1"
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "newobj",
          "patching_rect": [
            30,
            215,
            150,
            22
          ],
          "text": "route universe changed"
        }
      },
      {
        "box": {
          "id": "obj-11",
          "maxclass": "newobj",
          "patching_rect": [
            30,
            275,
            135,
            22
          ],
          "text": "print monitor-universe"
        }
      },
      {
        "box": {
          "id": "obj-12",
          "maxclass": "newobj",
          "patching_rect": [
            190,
            275,
            130,
            22
          ],
          "text": "print monitor-changed"
        }
      },
      {
        "box": {
          "id": "obj-13",
          "maxclass": "newobj",
          "patching_rect": [
            350,
            215,
            125,
            22
          ],
          "text": "print monitor-status"
        }
      },
      {
        "box": {
          "id": "obj-14",
          "maxclass": "comment",
          "patching_rect": [
            30,
            340,
            720,
            36
          ],
          "text": "Left outlet emits universe <id> <512 bytes> or changed <id> address value ...; right outlet emits status/errors."
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
            "obj-9",
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
            "obj-9",
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
            "obj-9",
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
            "obj-9",
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
            "obj-9",
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
            "obj-9",
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
            "obj-10",
            1
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
            "obj-9",
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
