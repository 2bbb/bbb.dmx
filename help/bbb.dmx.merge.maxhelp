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
      100,
      100,
      880,
      560
    ],
    "bglocked": 1,
    "default_fontsize": 12.0,
    "default_fontface": 0,
    "default_fontname": "Arial",
    "description": "bbb.dmx.merge — merge multiple DMX layers across universes",
    "digest": "bbb.dmx.merge — merge multiple DMX layers across universes",
    "tags": "dmx, lighting",
    "style": "",
    "boxes": [
      {
        "box": {
          "id": "obj-1",
          "maxclass": "comment",
          "patching_rect": [
            30,
            20,
            720,
            24
          ],
          "text": "bbb.dmx.merge — merge multiple DMX layers across universes"
        }
      },
      {
        "box": {
          "id": "obj-2",
          "maxclass": "comment",
          "patching_rect": [
            30,
            50,
            760,
            36
          ],
          "text": "Modes: priority, htp, ltp. Inputs use layer/universe/channel messages and output universe <id> <512 bytes>."
        }
      },
      {
        "box": {
          "id": "obj-3",
          "maxclass": "message",
          "patching_rect": [
            30,
            110,
            210,
            22
          ],
          "text": "mode priority"
        }
      },
      {
        "box": {
          "id": "obj-4",
          "maxclass": "message",
          "patching_rect": [
            260,
            110,
            210,
            22
          ],
          "text": "priority manual 100"
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "message",
          "patching_rect": [
            490,
            110,
            210,
            22
          ],
          "text": "channel base 1 1 64"
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "message",
          "patching_rect": [
            30,
            145,
            210,
            22
          ],
          "text": "channel manual 1 1 255"
        }
      },
      {
        "box": {
          "id": "obj-7",
          "maxclass": "message",
          "patching_rect": [
            260,
            145,
            210,
            22
          ],
          "text": "mode htp"
        }
      },
      {
        "box": {
          "id": "obj-8",
          "maxclass": "message",
          "patching_rect": [
            490,
            145,
            210,
            22
          ],
          "text": "mode ltp"
        }
      },
      {
        "box": {
          "id": "obj-9",
          "maxclass": "message",
          "patching_rect": [
            30,
            180,
            210,
            22
          ],
          "text": "bangall"
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "message",
          "patching_rect": [
            260,
            180,
            210,
            22
          ],
          "text": "clear all"
        }
      },
      {
        "box": {
          "id": "obj-20",
          "maxclass": "newobj",
          "patching_rect": [
            30,
            250,
            260,
            22
          ],
          "text": "bbb.dmx.merge"
        }
      },
      {
        "box": {
          "id": "obj-21",
          "maxclass": "newobj",
          "patching_rect": [
            30,
            310,
            160,
            22
          ],
          "text": "print bbb.dmx.merge-out"
        }
      },
      {
        "box": {
          "id": "obj-22",
          "maxclass": "newobj",
          "patching_rect": [
            260,
            310,
            170,
            22
          ],
          "text": "print bbb.dmx.merge-status"
        }
      },
      {
        "box": {
          "id": "obj-30",
          "maxclass": "comment",
          "patching_rect": [
            30,
            370,
            760,
            24
          ],
          "text": "priority: highest layer priority wins per valid channel, including zero values."
        }
      },
      {
        "box": {
          "id": "obj-31",
          "maxclass": "comment",
          "patching_rect": [
            30,
            405,
            760,
            24
          ],
          "text": "htp: highest value wins, useful for dimmer-like data."
        }
      },
      {
        "box": {
          "id": "obj-32",
          "maxclass": "comment",
          "patching_rect": [
            30,
            440,
            760,
            24
          ],
          "text": "ltp: most recently updated channel wins, useful for color/gobo/manual overrides."
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
            "obj-20",
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
            "obj-20",
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
            "obj-20",
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
            "obj-20",
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
            "obj-20",
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
            "obj-20",
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
            "obj-20",
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
            "obj-20",
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
            "obj-21",
            0
          ]
        }
      },
      {
        "patchline": {
          "source": [
            "obj-20",
            1
          ],
          "destination": [
            "obj-22",
            0
          ]
        }
      }
    ]
  }
}
