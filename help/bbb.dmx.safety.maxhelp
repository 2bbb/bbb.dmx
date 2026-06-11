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
    "description": "bbb.dmx.safety — clamp/freeze/blackout/deadman for multi-universe DMX",
    "digest": "bbb.dmx.safety — clamp/freeze/blackout/deadman for multi-universe DMX",
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
          "text": "bbb.dmx.safety — clamp/freeze/blackout/deadman for multi-universe DMX"
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
          "text": "Applies max_value, max_delta slew limiting, freeze, blackout, and timeout blackout to universe inputs."
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
          "text": "max_value 200"
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
          "text": "max_delta 20"
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
          "text": "timeout_ms 1000"
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
          "text": "channel 1 1 255"
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
          "text": "blackout 1"
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
          "text": "blackout 0"
        }
      },
      {
        "box": {
          "id": "obj-9",
          "maxclass": "message",
          "patching_rect": [
            30,
            180,
            230,
            22
          ],
          "text": "freeze 1"
        }
      },
      {
        "box": {
          "id": "obj-10",
          "maxclass": "message",
          "patching_rect": [
            275,
            180,
            230,
            22
          ],
          "text": "bangall"
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
          "text": "bbb.dmx.safety"
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
          "text": "print bbb.dmx.safety-out"
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
          "text": "print bbb.dmx.safety-status"
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
          "text": "Input form: universe <id> <512 values>, channel <universe> <address> <value>, channels <universe> <address> <value> ..."
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
          "text": "timeout_ms > 0 enables deadman blackout after input loss."
        }
      },
      {
        "box": {
          "id": "obj-32",
          "maxclass": "comment",
          "patching_rect": [
            30,
            450,
            780,
            24
          ],
          "text": "freeze holds the current protected output while still allowing bang output."
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
