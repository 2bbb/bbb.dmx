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
      900,
      580
    ],
    "bglocked": 1,
    "default_fontsize": 12.0,
    "default_fontface": 0,
    "default_fontname": "Arial",
    "description": "bbb.dmx.generator — generate multi-universe DMX test patterns",
    "digest": "bbb.dmx.generator — generate multi-universe DMX test patterns",
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
            760,
            24
          ],
          "text": "bbb.dmx.generator — generate multi-universe DMX test patterns"
        }
      },
      {
        "box": {
          "id": "obj-2",
          "maxclass": "comment",
          "patching_rect": [
            30,
            50,
            780,
            36
          ],
          "text": "Generates universe <id> <512 bytes> messages for address checks, ramps, and blackouts."
        }
      },
      {
        "box": {
          "id": "obj-3",
          "maxclass": "message",
          "patching_rect": [
            30,
            110,
            230,
            22
          ],
          "text": "blackout 1"
        }
      },
      {
        "box": {
          "id": "obj-4",
          "maxclass": "message",
          "patching_rect": [
            275,
            110,
            230,
            22
          ],
          "text": "full 1"
        }
      },
      {
        "box": {
          "id": "obj-5",
          "maxclass": "message",
          "patching_rect": [
            520,
            110,
            230,
            22
          ],
          "text": "all 1 127"
        }
      },
      {
        "box": {
          "id": "obj-6",
          "maxclass": "message",
          "patching_rect": [
            30,
            145,
            230,
            22
          ],
          "text": "range 1 1 16 255"
        }
      },
      {
        "box": {
          "id": "obj-7",
          "maxclass": "message",
          "patching_rect": [
            275,
            145,
            230,
            22
          ],
          "text": "ramp 1 1 32 0 255"
        }
      },
      {
        "box": {
          "id": "obj-8",
          "maxclass": "message",
          "patching_rect": [
            520,
            145,
            230,
            22
          ],
          "text": "chase 1 42 255"
        }
      },
      {
        "box": {
          "id": "obj-20",
          "maxclass": "newobj",
          "patching_rect": [
            30,
            260,
            260,
            22
          ],
          "text": "bbb.dmx.generator"
        }
      },
      {
        "box": {
          "id": "obj-21",
          "maxclass": "newobj",
          "patching_rect": [
            30,
            320,
            160,
            22
          ],
          "text": "print bbb.dmx.generator-out"
        }
      },
      {
        "box": {
          "id": "obj-22",
          "maxclass": "newobj",
          "patching_rect": [
            280,
            320,
            170,
            22
          ],
          "text": "print bbb.dmx.generator-status"
        }
      },
      {
        "box": {
          "id": "obj-30",
          "maxclass": "comment",
          "patching_rect": [
            30,
            380,
            780,
            24
          ],
          "text": "Use this for setup/address debugging, not as a show-control engine."
        }
      },
      {
        "box": {
          "id": "obj-31",
          "maxclass": "comment",
          "patching_rect": [
            30,
            415,
            780,
            24
          ],
          "text": "range/ramp/chase start from a zeroed universe and output one full universe message."
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
