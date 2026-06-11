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
    "description": "bbb.dmx.fade — fade DMX channels and universes over time",
    "digest": "bbb.dmx.fade — fade DMX channels and universes over time",
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
          "text": "bbb.dmx.fade — fade DMX channels and universes over time"
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
          "text": "Use channel/universe/channels messages. Output is universe <id> <512 faded bytes> at @fps while fades are active."
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
          "text": "channel 1 1 255 1000"
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
          "text": "channels 1 500 1 0 2 255"
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
          "text": "fps 40"
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
          "text": "time_ms 2000"
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
          "text": "bangall"
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
          "text": "stop"
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
          "text": "clear"
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
          "text": "bbb.dmx.fade"
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
          "text": "print bbb.dmx.fade-out"
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
          "text": "print bbb.dmx.fade-status"
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
          "text": "universe message form: universe <id> <time_ms> <512 target values>."
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
          "text": "Bare 512-value list uses @universe and @time_ms."
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
          "text": "stop freezes at current values; clear removes state."
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
