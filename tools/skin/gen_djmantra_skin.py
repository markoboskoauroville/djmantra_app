#!/usr/bin/env python3
"""Generates the DJ Mantra skin: res/skins/DJMantra/{skin.xml,style.qss,svg/*}.

The layout follows djay's phone layout (docs/ui-reference/*.png) with the
turntables replaced by album art:

  landscape                               portrait
  ---------                               --------
  header: cover, artist/title, key,       library button
          time, overview | button | deck 2 header: deck 1 | deck 2 (cover, text,
  middle: [mixer | waveforms | pads]              overview)
          mixer/pads: deck 1 (SYNC, BPM,  tabs: mixer | waveforms | pads
          pitch, cover) | centre | deck 2 middle: the selected page (or deck 1/2)
          waveforms: horizontal, stacked  selector: 1 | Mix | 2
  bottom: play, SET, return, key lock,    transport: SYNC/BPM, SET/return,
          crossfader                      play + crossfader

A double tap on a waveform shows the waveforms full screen (vertical in
portrait, horizontal in landscape); another double tap goes back.

Portrait or landscape is picked by a SizeAwareStack from the window width.
Run it after changing the layout:  tools/skin/gen_djmantra_skin.py
"""
import os
import shutil

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SKIN = os.path.join(ROOT, "res", "skins", "DJMantra")
P = "skins:DJMantra/svg/"

# Colours measured from djay Pro (Marko's reference, 10.10.2026): charcoal
# panels, near-black areas, buttons a shade lighter with a near-black outline
BG = "#1e2021"
PANEL = "#1e2021"
TOPBAR = "#0e1011"
CENTER = "#191b1c"
GREY = "#8a8d90"
BUTTON = "#1f2122"
OUTLINE = "#0b0c0d"
PRESSED = "#2e3133"
WHITE = "#f2f2f2"
BLUE = "#2f8cff"
GREEN = "#3ad13a"
MAGENTA = "#e04ad0"
RED = "#ff2a2a"

DECK_KEY_COLOR = {1: GREEN, 2: MAGENTA}


def grp(n):
    return "[Channel%d]" % n


# --------------------------------------------------------------------------
# XML helpers


def group(children, layout="horizontal", name=None, policy=None, size=None,
          minsize=None, maxsize=None, extra=""):
    out = ["<WidgetGroup>"]
    if name:
        out.append("<ObjectName>%s</ObjectName>" % name)
    out.append("<Layout>%s</Layout>" % layout)
    if size:
        out.append("<Size>%s</Size>" % size)
    if policy:
        out.append("<SizePolicy>%s</SizePolicy>" % policy)
    if minsize:
        out.append("<MinimumSize>%s</MinimumSize>" % minsize)
    if maxsize:
        out.append("<MaximumSize>%s</MaximumSize>" % maxsize)
    out.append(extra)
    out.append("<Children>")
    out.extend(children)
    out.append("</Children></WidgetGroup>")
    return "\n".join(out)


def hbox(*children, **kw):
    return group(list(children), "horizontal", **kw)


def vbox(*children, **kw):
    return group(list(children), "vertical", **kw)


def spacer(policy="me,min", size=None):
    """A horizontal stretch (vstretch() for a vertical one)."""
    s = "<WidgetGroup><Layout>horizontal</Layout>"
    if size:
        s += "<Size>%s</Size>" % size
    else:
        s += "<SizePolicy>%s</SizePolicy>" % policy
    return s + "</WidgetGroup>"


def vstretch():
    return spacer("min,me")


def hspace(px):
    return spacer(size="%df,0min" % px)


def vspace(px):
    return spacer(size="0min,%df" % px)


def label(text, name="Caption", size=None, align="center"):
    s = "<Label><ObjectName>%s</ObjectName><Text>%s</Text><Alignment>%s</Alignment>" % (
        name, text, align)
    if size:
        s += "<Size>%s</Size>" % size
    return s + "</Label>"


def button(key, name, text="", size="60f,40f", states=2, right_key=None,
           pixmaps=None, display_key=None, tooltip=None):
    """A PushButton; text buttons are drawn by the QSS, others by pixmaps:
    pixmaps = [(unpressed, pressed), ...] per state."""
    out = ["<PushButton>", "<ObjectName>%s</ObjectName>" % name,
           "<Size>%s</Size>" % size, "<NumberStates>%d</NumberStates>" % states,
           "<RightClickIsPushButton>false</RightClickIsPushButton>"]
    if tooltip:
        out.append("<TooltipId>%s</TooltipId>" % tooltip)
    for i in range(states):
        out.append("<State><Number>%d</Number>" % i)
        if text:
            out.append("<Text>%s</Text><Alignment>center</Alignment>" % text)
        if pixmaps:
            un, pr = pixmaps[min(i, len(pixmaps) - 1)]
            out.append('<Unpressed scalemode="STRETCH_ASPECT">%s%s</Unpressed>' % (P, un))
            out.append('<Pressed scalemode="STRETCH_ASPECT">%s%s</Pressed>' % (P, pr))
        out.append("</State>")
    out.append("<Connection><ConfigKey>%s</ConfigKey><ButtonState>LeftButton</ButtonState>"
               "</Connection>" % key)
    if right_key:
        out.append("<Connection><ConfigKey>%s</ConfigKey><ButtonState>RightButton</ButtonState>"
                   "</Connection>" % right_key)
    if display_key:
        out.append("<Connection><ConfigKey>%s</ConfigKey><ConnectControl>false</ConnectControl>"
                   "</Connection>" % display_key)
    out.append("</PushButton>")
    return "\n".join(out)


def knob(key, size=40, bipolar=True, name="Knob"):
    return """<KnobComposed>
  <ObjectName>%s</ObjectName>
  <Size>%df,%df</Size>
  <Knob>%sknob_indicator.svg</Knob>
  <BackPath>%sknob_bg.svg</BackPath>
  <MinAngle>-135</MinAngle>
  <MaxAngle>135</MaxAngle>
  <ArcRadius>%.1f</ArcRadius>
  <ArcThickness>2.5</ArcThickness>
  <ArcBgThickness>2.5</ArcBgThickness>
  <ArcColor>%s</ArcColor>
  <ArcBgColor>#2e3133</ArcBgColor>
  <ArcUnipolar>%s</ArcUnipolar>
  <ArcRoundCaps>true</ArcRoundCaps>
  <Connection><ConfigKey>%s</ConfigKey></Connection>
</KnobComposed>""" % (name, size, size, P, P, size / 2.0 - 2, "#4a4d50",
                     "false" if bipolar else "true", key)


def knob_with_label(key, text, size=40):
    return vbox(hbox(spacer(), knob(key, size), spacer(), policy="me,min"),
                label(text, "KnobLabel"), policy="me,min")


def eq_key(n, param):
    return "[EqualizerRack1_%s_Effect1],parameter%d" % (grp(n), param)


def filter_key(n):
    return "[QuickEffectRack1_%s],super1" % grp(n)


def slider_v(key, name="ChannelFader", size="44f,-1me", handle="fader_handle.svg",
             track="fader_track.svg"):
    return """<SliderComposed>
  <ObjectName>%s</ObjectName>
  <Size>%s</Size>
  <Handle scalemode="STRETCH_ASPECT">%s%s</Handle>
  <Slider scalemode="STRETCH">%s%s</Slider>
  <Horizontal>false</Horizontal>
  <Connection><ConfigKey>%s</ConfigKey></Connection>
</SliderComposed>""" % (name, size, P, handle, P, track, key)


def crossfader():
    return """<SliderComposed>
  <ObjectName>Crossfader</ObjectName>
  <Size>-1me,48f</Size>
  <Handle scalemode="STRETCH_ASPECT">%sxfader_handle.svg</Handle>
  <Slider scalemode="STRETCH">%sxfader_track.svg</Slider>
  <Horizontal>true</Horizontal>
  <Connection><ConfigKey>[Master],crossfader</ConfigKey></Connection>
</SliderComposed>""" % (P, P)


def vumeter(n, size="10f,-1me"):
    return """<VuMeter>
  <Size>%s</Size>
  <PathBack scalemode="STRETCH">%svu_back.svg</PathBack>
  <PathVu scalemode="STRETCH">%svu_on.svg</PathVu>
  <Horizontal>false</Horizontal>
  <PeakHoldSize>3</PeakHoldSize>
  <PeakHoldTime>400</PeakHoldTime>
  <PeakFallTime>20</PeakFallTime>
  <PeakFallStep>2</PeakFallStep>
  <Connection><ConfigKey>%s,vu_meter</ConfigKey></Connection>
</VuMeter>""" % (size, P, P, grp(n))


def cover(n, size="-1me,-1me", minsize="40,40", name="Cover"):
    return """<CoverArt>
  <ObjectName>%s</ObjectName>
  <Size>%s</Size>
  <MinimumSize>%s</MinimumSize>
  <Group>%s</Group>
  <DefaultCover>%scover_default.svg</DefaultCover>%s
</CoverArt>""" % (name, size, minsize, grp(n), P,
                  # the header's note icons open the deck's song picker
                  "\n  <OpensTrackPicker>true</OpensTrackPicker>" if name == "HeaderCover" else
                  # One Deck: the album art picks the folder
                  "\n  <OpensTrackPicker>folders</OpensTrackPicker>" if name == "OneDeckCover"
                  else "")


def track_prop(n, prop, name, align="left"):
    return """<TrackProperty>
  <ObjectName>%s</ObjectName>
  <Size>-1me,-1min</Size>
  <Property>%s</Property>
  <Alignment>%s</Alignment>
  <Elide>right</Elide>
  <Channel>%d</Channel>
</TrackProperty>""" % (name, prop, align, n)


def time_remaining(n, align="right"):
    return """<NumberPos>
  <ObjectName>TimeText</ObjectName>
  <Size>-1min,-1min</Size>
  <Alignment>%s</Alignment>
  <Channel>%d</Channel>
  <Connection><ConfigKey>%s,playposition</ConfigKey></Connection>
</NumberPos>""" % (align, n, grp(n))


def key_label(n, align="right"):
    return """<Key>
  <ObjectName>KeyDeck%d</ObjectName>
  <Group>%s</Group>
  <Size>-1min,-1min</Size>
  <Alignment>%s</Alignment>
  <Connection><ConfigKey>%s,visual_key</ConfigKey></Connection>
</Key>""" % (n, grp(n), align, grp(n))


def bpm(n, align="center", size="-1me,-1min"):
    return """<Number>
  <ObjectName>BpmText</ObjectName>
  <Size>%s</Size>
  <Alignment>%s</Alignment>
  <NumberOfDigits>1</NumberOfDigits>
  <Connection><ConfigKey>%s,visual_bpm</ConfigKey></Connection>
</Number>""" % (size, align, grp(n))


def overview(n, height=26):
    # height None: as tall as there is room
    return """<Overview>
  <ObjectName>Overview</ObjectName>
  <Size>-1me,%s</Size>
  <Group>%s</Group>
  <BgColor>%s</BgColor>
  <SignalColor>#ff4040</SignalColor>
  <PlayedOverlayColor>#60000000</PlayedOverlayColor>
  <PlayPosColor>%s</PlayPosColor>
  <EndOfTrackColor>#ff6600</EndOfTrackColor>
  <LabelFontSize>9</LabelFontSize>
  <DefaultMark>
    <Align>bottom|right</Align>
    <Color>#ff8800</Color>
    <TextColor>#ffffff</TextColor>
    <Text> %%1 </Text>
  </DefaultMark>
  <Mark>
    <Control>cue_point</Control>
    <Align>top|right</Align>
    <Color>#ffffff</Color>
    <TextColor>#000000</TextColor>
  </Mark>
  <MarkRange>
    <StartControl>loop_start_position</StartControl>
    <EndControl>loop_end_position</EndControl>
    <EnabledControl>loop_enabled</EnabledControl>
    <Color>#3ad13a</Color>
    <Opacity>0.5</Opacity>
    <DisabledColor>#ffffff</DisabledColor>
    <DisabledOpacity>0.3</DisabledOpacity>
  </MarkRange>
  <Connection><ConfigKey>%s,playposition</ConfigKey></Connection>
</Overview>""" % ("-1me" if height is None else "%df" % height, grp(n), TOPBAR, RED, grp(n))


def visual(n, vertical):
    return """<Visual>
  <ObjectName>Waveform</ObjectName>
  <TooltipId>waveform_display</TooltipId>
  <SizePolicy>me,me</SizePolicy>
  <Channel>%d</Channel>
  <Orientation>%s</Orientation>
  <BgColor>#000000</BgColor>
  <SignalColor>#ff4040</SignalColor>
  <BeatColor>#5a5a60</BeatColor>
  <AxesColor>#00000000</AxesColor>
  <PlayPosColor>%s</PlayPosColor>
  <EndOfTrackColor>#ff6600</EndOfTrackColor>
  <DefaultMark>
    <Align>bottom|right</Align>
    <Color>#ff8800</Color>
    <TextColor>#ffffff</TextColor>
    <Text> %%1 </Text>
  </DefaultMark>
  <Mark>
    <Control>cue_point</Control>
    <Text>CUE</Text>
    <Align>top|right</Align>
    <Color>#ffffff</Color>
    <TextColor>#000000</TextColor>
  </Mark>
  <MarkRange>
    <StartControl>loop_start_position</StartControl>
    <EndControl>loop_end_position</EndControl>
    <EnabledControl>loop_enabled</EnabledControl>
    <Color>#3ad13a</Color>
    <Opacity>0.4</Opacity>
    <DisabledColor>#ffffff</DisabledColor>
    <DisabledOpacity>0.2</DisabledOpacity>
  </MarkRange>
</Visual>""" % (n, "vertical" if vertical else "horizontal", RED)


# --------------------------------------------------------------------------
# Controls built from the helpers


def play(n, size=56):
    return button("%s,play" % grp(n), "PlayButton", size="%df,%df" % (size, size),
                  pixmaps=[("play_off.svg", "play_pressed.svg"),
                           ("play_on.svg", "play_pressed.svg")],
                  display_key="%s,play_indicator" % grp(n), tooltip="play_cue_set")


def cue_pair(n, width=150, height=44):
    """The controller's "cue" button (set the cue point, back to it, hold to
    preview), the same size as before."""
    return hbox(
        button("%s,cue_default" % grp(n), "CueButton", "CUE", "%df,%df" % (width, height),
               display_key="%s,cue_indicator" % grp(n), tooltip="cue_default_cue_gotoandstop"),
        name="CuePair", policy="min,min")


def sync(n, size="70f,40f"):
    return button("%s,sync_enabled" % grp(n), "SyncButton", "SYNC", size)


def keylock(n, size=44):
    return button("%s,keylock" % grp(n), "RoundButton", "&#9835;", "%df,%df" % (size, size),
                  tooltip="keylock")


def bend(n):
    return hbox(
        button("%s,rate_temp_down" % grp(n), "BendButton", "&#8722;", "36f,30f", states=1),
        button("%s,rate_temp_up" % grp(n), "BendButton", "+", "36f,30f", states=1),
        policy="min,min")


def pitch(n, size="44f,-1me"):
    return slider_v("%s,rate" % grp(n), "PitchFader", size, "pitch_handle.svg", "pitch_track.svg")


def loop_row(n):
    return hbox(
        spacer(),
        button("%s,waveform_zoom_down" % grp(n), "IconButton", "&#8722;", "34f,34f", states=1),
        button("%s,loop_halve" % grp(n), "IconButton", "&lt;", "34f,34f", states=1),
        button("%s,beatloop_activate" % grp(n), "LoopButton", "", "52f,34f",
               states=2, display_key="%s,loop_enabled" % grp(n),
               pixmaps=[("loop.svg", "loop_on.svg"), ("loop_on.svg", "loop_on.svg")]),
        """<Number>
  <ObjectName>LoopSize</ObjectName>
  <Size>26f,34f</Size>
  <Alignment>center</Alignment>
  <NumberOfDigits>0</NumberOfDigits>
  <Connection><ConfigKey>%s,beatloop_size</ConfigKey></Connection>
</Number>""" % grp(n),
        button("%s,loop_double" % grp(n), "IconButton", "&gt;", "34f,34f", states=1),
        button("%s,waveform_zoom_up" % grp(n), "IconButton", "+", "34f,34f", states=1),
        spacer(), name="LoopRow", policy="me,min")


PAD_MODES = (("hotcue", "HOT CUE"), ("loop", "LOOP"), ("fx", "FX"), ("sampler", "SAMPLER"))
LOOP_SIZES = (("0.25", "&#188;"), ("0.5", "&#189;"), ("1", "1"), ("2", "2"),
              ("4", "4"), ("8", "8"), ("16", "16"), ("32", "32"))


def pad_button(key, text, display_key=None, states=2):
    return """<PushButton>
  <ObjectName>PadButton</ObjectName>
  <SizePolicy>me,me</SizePolicy>
  <MinimumSize>36,36</MinimumSize>
  <NumberStates>%d</NumberStates>
  %s
  <Connection><ConfigKey>%s</ConfigKey><ButtonState>LeftButton</ButtonState></Connection>%s
</PushButton>""" % (states, "".join("<State><Number>%d</Number><Text>%s</Text></State>" % (i, text)
                              for i in range(states)), key,
                    ("<Connection><ConfigKey>%s</ConfigKey><ConnectControl>false</ConnectControl>"
                     "</Connection>" % display_key) if display_key else "")


def pad_cells(n, mode):
    g = grp(n)
    cells = []
    for i in range(8):
        num = i + 1
        if mode == "hotcue":
            cells.append("""<HotcueButton>
  <ObjectName>Pad</ObjectName>
  <SizePolicy>me,me</SizePolicy>
  <MinimumSize>36,36</MinimumSize>
  <Group>%s</Group>
  <Hotcue>%d</Hotcue>
  <NumberStates>3</NumberStates>
  <State><Number>0</Number><Text>%d</Text></State>
  <State><Number>1</Number><Text>%d</Text></State>
  <State><Number>2</Number><Text>%d</Text></State>
</HotcueButton>""" % (g, num, num, num, num))
        elif mode == "loop":
            size, text = LOOP_SIZES[i]
            cells.append(pad_button("%s,beatloop_%s_toggle" % (g, size), text,
                                    "%s,beatloop_%s_enabled" % (g, size)))
        elif mode == "fx":
            unit = "[EffectRack1_EffectUnit%d" % n
            if i < 3:
                cells.append(pad_button("%s_Effect%d],enabled" % (unit, num), "FX%d" % num))
            elif i == 3:
                cells.append(pad_button("%s],group_%s_enable" % (unit, g), "ON"))
            else:
                jump = (("4", "backward", "&#171; 4"), ("1", "backward", "&#8249; 1"),
                        ("1", "forward", "1 &#8250;"), ("4", "forward", "4 &#187;"))[i - 4]
                cells.append(pad_button("%s,beatjump_%s_%s" % (g, jump[0], jump[1]), jump[2],
                                        states=1))
        else:  # sampler
            cells.append(pad_button("[Sampler%d],cue_gotoandplay" % num, "S%d" % num,
                                    "[Sampler%d],play_indicator" % num, states=1))
    return vbox(hbox(*cells[:4], policy="me,me"), hbox(*cells[4:], policy="me,me"),
                policy="me,me")


def pads(n, column=True):
    """A deck's lower half as on the controller: the mode buttons (hot cue,
    loop, fx, sampler), sync / cue / play on the left, the 8 pads."""
    modes = hbox(*[button("[DJMantra],d%d_%s" % (n, key), "ModeButton", text, "-1me,28f")
                   for key, text in PAD_MODES], name="ModeRow", policy="me,min")
    pages = stack([("[DJMantra],d%d_%s" % (n, key), vbox(pad_cells(n, key), policy="me,me"), 0)
                   for key, _ in PAD_MODES])
    left = vbox(sync(n, "62f,34f"), vspace(4),
                button("%s,cue_default" % grp(n), "CueButton", "CUE", "62f,-1me",
                       display_key="%s,cue_indicator" % grp(n)),
                vspace(4),
                button("%s,play" % grp(n), "PadPlay", "&#9654;/II", "62f,-1me",
                       display_key="%s,play_indicator" % grp(n)),
                policy="min,me")
    # without the column where the transport row below already has them
    body = hbox(left, hspace(4), pages, policy="me,me") if column else pages
    return vbox(label("DECK %d" % n, "PadsTitle", align="left"), modes, vspace(4), body,
                name="PadsDeck", policy="me,me")


def header_deck_landscape(n):
    text = vbox(
        hbox(track_prop(n, "artist", "ArtistText"), key_label(n), policy="me,min"),
        hbox(track_prop(n, "title", "TitleText"), time_remaining(n), policy="me,min"),
        overview(n, 22),
        policy="me,max")
    art = cover(n, "58f,58f", "58,58", "HeaderCover")
    return hbox(*((art, hspace(8), text) if n == 1 else (text, hspace(8), art)),
                policy="me,max")


def header_deck_portrait(n):
    if n == 1:
        time_key = hbox(time_remaining(n, "left"), spacer(), key_label(n), policy="me,min")
        text = vbox(track_prop(n, "artist", "ArtistText"),
                    track_prop(n, "title", "TitleText"), time_key, policy="me,min")
        top = hbox(cover(n, "58f,58f", "58,58", "HeaderCover"), hspace(6), text,
                   policy="me,min")
    else:
        time_key = hbox(key_label(n, "left"), spacer(), time_remaining(n), policy="me,min")
        text = vbox(track_prop(n, "artist", "ArtistText", "right"),
                    track_prop(n, "title", "TitleText", "right"), time_key, policy="me,min")
        top = hbox(text, hspace(6), cover(n, "58f,58f", "58,58", "HeaderCover"),
                   policy="me,min")
    return vbox(top, vspace(4), overview(n, 22), policy="me,max")


def library_button(size=40):
    # the menu sheet (djay's): Library, Controller, REC, Settings
    return button("[DJMantra],show_menu", "LibraryButton", "", "%df,%df" % (size, size),
                  pixmaps=[("dj_logo.svg", "dj_logo.svg"), ("dj_logo.svg", "dj_logo.svg")])


def tabs(prefix):
    t = []
    for page, icon in (("mixer", "tab_mixer"), ("waveforms", "tab_wave"), ("pads", "tab_pads")):
        t.append(button("[DJMantra],%s_%s" % (prefix, page), "TabButton", "", "60f,34f",
                        pixmaps=[("%s.svg" % icon, "%s_on.svg" % icon),
                                 ("%s_on.svg" % icon, "%s_on.svg" % icon)]))
    return hbox(spacer(), *t, spacer(), name="TabRow", policy="me,max")


def deck_controls_column(n):
    """SYNC, BPM, pitch fader (landscape deck column)."""
    return vbox(
        sync(n, "72f,36f"),
        label("BPM", "BpmCaption"),
        bpm(n),
        vspace(4),
        hbox(spacer(), pitch(n), spacer(), policy="me,me"),
        name="DeckControls", policy="min,me", size="78f,-1me")


def deck_landscape(n):
    art = vbox(cover(n, "-1me,-1me", "120,120", "DeckCover"),
               hbox(*((spacer(), bend(n)) if n == 1 else (bend(n), spacer())), policy="me,min"),
               policy="me,me")
    parts = (deck_controls_column(n), hspace(6), art) if n == 1 else \
        (art, hspace(6), deck_controls_column(n))
    return hbox(*parts, name="Deck", policy="me,me")


def pfl(n, size="40f,30f"):
    """The controller's headphone buttons 1 and 2 (pre-listen)."""
    return button("%s,pfl" % grp(n), "PflButton", "&#127911;%d" % n, size)


def load_button(n, size="40f,30f"):
    """The controller's buttons 1 and 2 next to the browser: a deck's songs."""
    return button("[DJMantra],load_deck%d" % n, "LoadDeckButton", "%d" % n, size, states=1)


def eq_column(n, big=34, filt=38):
    """The controller's knob column of a deck: high/gain, mid, low, filter."""
    return vbox(knob_with_label(eq_key(n, 3), "HIGH / GAIN", big),
                vstretch(), knob_with_label(eq_key(n, 2), "MID", big),
                vstretch(), knob_with_label(eq_key(n, 1), "LOW", big),
                vstretch(), knob_with_label(filter_key(n), "FILTER", filt),
                policy="min,me")


def mixer_centre(fader_width=30, meter_width=10, button="40f,30f"):
    """Between the knob columns, as on the controller: 1 and 2, master,
    the headphone buttons, then the volume faders with their meters."""
    return vbox(
        hbox(spacer(), load_button(1, button), hspace(4), load_button(2, button), spacer(),
             policy="me,min"),
        knob_with_label("[Master],gain", "MASTER", 30),
        hbox(spacer(), pfl(1, button), hspace(4), pfl(2, button), spacer(), policy="me,min"),
        vspace(4),
        hbox(slider_v("%s,volume" % grp(1), size="%df,-1me" % fader_width),
             vumeter(1, "%df,-1me" % meter_width), hspace(2),
             vumeter(2, "%df,-1me" % meter_width),
             slider_v("%s,volume" % grp(2), size="%df,-1me" % fader_width), policy="me,me"),
        policy="me,me")


def mixer_landscape():
    return hbox(eq_column(1), hspace(2), mixer_centre(26, 8, "32f,28f"), hspace(2), eq_column(2),
                name="Mixer", policy="me,me")


def mixer_portrait():
    return hbox(eq_column(1, 42, 46), hspace(6), mixer_centre(56, 12), hspace(6),
                eq_column(2, 42, 46), name="Mixer", policy="me,me")


def deck_page_portrait(n):
    art = cover(n, "-1me,-1me", "160,160", "DeckCover")
    side = vbox(label("BPM", "BpmCaption"), bpm(n), pitch(n, "50f,-1me"), bend(n),
                policy="min,me", name="DeckControls")
    body = hbox(*((side, art) if n == 1 else (art, side)), policy="me,me")
    return vbox(body, loop_row(n), name="Deck", policy="me,me")


def waveforms(vertical):
    if vertical:
        body = hbox(visual(1, True), spacer(size="1f,-1me"), visual(2, True),
                    name="WaveformArea", policy="me,me")
        loops = hbox(loop_row(1), loop_row(2), policy="me,min")
        return vbox(body, loops, policy="me,me")
    return vbox(visual(1, False), vspace(1), visual(2, False),
                hbox(loop_row(1), loop_row(2), policy="me,min"),
                name="WaveformArea", policy="me,me")


def stack(pages, current_key=None):
    """pages = [(trigger key or None, xml, on_hide_select or None)]"""
    attr = ' currentpage="%s"' % current_key if current_key else ""
    out = ["<WidgetStack%s>" % attr, "<SizePolicy>me,me</SizePolicy>", "<Children>"]
    for trigger, xml, on_hide in pages:
        a = ""
        if trigger:
            a += ' trigger="%s"' % trigger
        if on_hide is not None:
            a += ' on_hide_select="%d"' % on_hide
        # each page in a group that carries the trigger
        out.append(xml.replace("<WidgetGroup>", "<WidgetGroup%s>" % a, 1))
    out.append("</Children></WidgetStack>")
    return "\n".join(out)


# --------------------------------------------------------------------------
# Landscape and portrait performance views


def performance_landscape():
    header = hbox(header_deck_landscape(1), hspace(12),
                  vbox(vstretch(), library_button(34), vstretch(), policy="min,me"),
                  hspace(12), header_deck_landscape(2),
                  name="TopBar", policy="me,max")

    def centre(page_xml):
        return vbox(tabs("l"), page_xml, name="Centre", policy="min,me", size="210f,-1me")

    def decks_with(centre_xml):
        return hbox(deck_landscape(1), centre_xml, deck_landscape(2), policy="me,me")

    middle = stack([
        ("[DJMantra],l_mixer", vbox(decks_with(centre(mixer_landscape())), policy="me,me"), 0),
        ("[DJMantra],l_waveforms", vbox(tabs("l"), waveforms(False), policy="me,me"), 0),
        ("[DJMantra],l_pads", vbox(tabs("l"),
                                   hbox(pads(1, False), hspace(12), pads(2, False), name="PadsArea",
                                        policy="me,me"),
                                   hbox(loop_row(1), loop_row(2), policy="me,min"),
                                   policy="me,me"), 0),
    ])

    def transport(n):
        parts = [play(n), hspace(10), cue_pair(n), hspace(10), keylock(n)]
        return hbox(*(parts if n == 1 else list(reversed(parts))), policy="min,min")
    bottom = hbox(transport(1), hspace(14), crossfader(), hspace(14), transport(2),
                  name="Transport", policy="me,min")
    return vbox(header, middle, bottom, name="Landscape", policy="me,me", minsize="700,0")


def performance_portrait():
    header = vbox(hbox(spacer(), library_button(32), spacer(), policy="me,min"),
                  hbox(header_deck_portrait(1), hspace(14), header_deck_portrait(2),
                       policy="me,min"),
                  name="TopBar", policy="me,max")
    middle = stack([
        ("[DJMantra],p_mixer", vbox(mixer_portrait(), policy="me,me"), 0),
        ("[DJMantra],p_waveforms", vbox(waveforms(True), policy="me,me"), 0),
        # deck 1's pads above deck 2's, each across the whole width: square pads
        ("[DJMantra],p_pads", vbox(pads(1, False), loop_row(1), vspace(6), pads(2, False), loop_row(2),
                                   name="PadsArea", policy="me,me"), 0),
        ("[DJMantra],p_deck1", vbox(deck_page_portrait(1), policy="me,me"), 0),
        ("[DJMantra],p_deck2", vbox(deck_page_portrait(2), policy="me,me"), 0),
    ])
    selector = hbox(
        button("[DJMantra],p_deck1", "SelectorButton", "1", "-1me,40f"),
        button("[DJMantra],p_mixer", "SelectorButton", "Mix", "-1me,40f"),
        button("[DJMantra],p_deck2", "SelectorButton", "2", "-1me,40f"),
        name="Selector", policy="me,min")
    row1 = hbox(sync(1), hspace(6), vbox(bpm(1, "left"), label("BPM", "BpmCaption", align="left"),
                                         policy="me,min"),
                keylock(1, 42),
                vbox(bpm(2, "right"), label("BPM", "BpmCaption", align="right"), policy="me,min"),
                hspace(6), sync(2), policy="me,min")
    row2 = hbox(cue_pair(1, 150, 42), spacer(),
                button("[DJMantra],waveform_fullscreen", "Chevron", "", "34f,34f",
                       pixmaps=[("chevron.svg", "chevron.svg"), ("chevron.svg", "chevron.svg")]),
                spacer(), cue_pair(2, 150, 42), policy="me,min")
    row3 = hbox(play(1, 58), hspace(12), crossfader(), hspace(12), play(2, 58), policy="me,min")
    transport = vbox(row1, vspace(6), row2, vspace(6), row3, name="Transport", policy="me,min")
    return vbox(header, tabs("p"), middle, selector, transport, name="Portrait",
                policy="me,me", maxsize="699,-1")


def fullscreen_waveforms():
    """Double tap on a waveform: only the waveforms (and a slim title row)."""
    def titles(n):
        return hbox(track_prop(n, "titleInfo", "TitleText"), time_remaining(n), policy="me,min")
    portrait = vbox(hbox(titles(1), hspace(10), titles(2), policy="me,min"),
                    hbox(visual(1, True), spacer(size="1f,-1me"), visual(2, True),
                         policy="me,me"),
                    name="FullWaveforms", policy="me,me", maxsize="699,-1")
    landscape = vbox(titles(1), visual(1, False), vspace(1), visual(2, False), titles(2),
                     name="FullWaveforms", policy="me,me", minsize="700,0")
    return size_aware(portrait, landscape)


def library_view():
    top = hbox(
        button("[DJMantra],show_library", "BackButton", "&#8249; DECKS", "100f,38f"),
        hspace(8),
        "<SearchBox></SearchBox>",
        hspace(8),
        button("[Channel1],LoadSelectedTrack", "LoadButton", "LOAD 1", "84f,38f", states=1),
        button("[Channel2],LoadSelectedTrack", "LoadButton", "LOAD 2", "84f,38f", states=1),
        hspace(6),
        button("[DJMantra],show_controller_map", "LoadButton", "CONTROLLER", "110f,38f", states=1),
        hspace(6),
        button("[DJMantra],show_preferences", "GearButton", "", "38f,38f", states=1,
               pixmaps=[("gear.svg", "gear.svg")]),
        name="LibraryTop", policy="me,min")
    body = """<Splitter>
  <ObjectName>LibrarySplitter</ObjectName>
  <Orientation>horizontal</Orientation>
  <SplitSizes>1,3</SplitSizes>
  <SplitSizesConfigKey>[DJMantra],library_split</SplitSizesConfigKey>
  <SizePolicy>me,me</SizePolicy>
  <Children>
    <LibrarySidebar></LibrarySidebar>
    <Library>
      <ShowButtonText>false</ShowButtonText>
    </Library>
  </Children>
</Splitter>"""
    return vbox(top, body, name="LibraryView", policy="me,me")


def one_deck_view():
    """djay's One Deck for the phone (Marko's screenshot of djay Pro): listening
    to the library. Album art (a tap: the folders), title, artist, BPM, key,
    time; the overview, the only waveform here (Marko, 10.10.2026: a tap
    jumps there); previous / play / next / cue; then
    the songs of the folder (a tap plays one, the next follows at the end).
    The list is put into "OneDeckList" by the main window."""
    def fact(caption, value):
        return vbox(label(caption, "OneDeckCaption", align="left"), value, policy="me,min")
    info = vbox(fact("TITLE", track_prop(1, "title", "OneDeckTitle")),
                fact("ARTIST", track_prop(1, "artist", "OneDeckArtist")),
                hbox(fact("BPM", bpm(1, "left")), fact("KEY", key_label(1, "left")),
                     fact("TIME", time_remaining(1, "left")), policy="me,min"),
                policy="me,min")
    top = hbox(cover(1, "104f,104f", "104,104", "OneDeckCover"), hspace(12), info,
               vbox(library_button(32), vstretch(), policy="min,me"),
               name="TopBar", size="-1me,124f")
    transport = hbox(
        button("[DJMantra],one_deck_prev", "IconButton", "", "48f,48f", states=1,
               pixmaps=[("prev.svg", "prev.svg")]),
        play(1, 52),
        button("[DJMantra],one_deck_next", "IconButton", "", "48f,48f", states=1,
               pixmaps=[("next.svg", "next.svg")]),
        hspace(14),
        button("%s,cue_default" % grp(1), "CueButton", "CUE", "96f,44f",
               display_key="%s,cue_indicator" % grp(1)),
        spacer(),
        # what happens at the end of a song: the next one in the folder
        # (CONTINUOUS, as radio playout calls it) or stop (Winamp's "stop
        # after current")
        """<PushButton>
  <ObjectName>ContinuousButton</ObjectName>
  <Size>118f,44f</Size>
  <NumberStates>2</NumberStates>
  <State><Number>0</Number><Text>STOP AFTER</Text></State>
  <State><Number>1</Number><Text>CONTINUOUS</Text></State>
  <Connection><ConfigKey>[DJMantra],one_deck_continuous</ConfigKey><ButtonState>LeftButton</ButtonState></Connection>
</PushButton>""",
        name="Transport", policy="me,min")
    portrait = vbox(top, overview(1, 148), transport,
                    vbox(name="OneDeckList", policy="me,me"),
                    name="OneDeck", policy="me,me", maxsize="699,-1")
    # landscape: the player on the left, the songs on the right
    player = vbox(top, overview(1, None), transport,
                  policy="me,me")
    landscape = hbox(player, vbox(name="OneDeckList", policy="me,me", size="380f,-1me"),
                     name="OneDeck", policy="me,me", minsize="700,0")
    return size_aware(portrait, landscape)


def size_aware(portrait, landscape):
    return ("<SizeAwareStack><SizePolicy>me,me</SizePolicy><Children>\n%s\n%s\n"
            "</Children></SizeAwareStack>" % (portrait, landscape))


# --------------------------------------------------------------------------
# skin.xml, style.qss, graphics


def skin_xml():
    attrs = {
        "[App],num_decks": "2",
        "[App],num_samplers": "8",
        "[Skin],show_spinnies": "0",
        "[Skin],show_coverart": "1",
        "[DJMantra],show_library": "0",
        "[DJMantra],waveform_fullscreen": "0",
        "[DJMantra],l_mixer": "1",
        "[DJMantra],l_waveforms": "0",
        "[DJMantra],l_pads": "0",
        "[DJMantra],p_mixer": "1",
        "[DJMantra],p_waveforms": "0",
        "[DJMantra],p_pads": "0",
        "[DJMantra],p_deck1": "0",
        "[DJMantra],p_deck2": "0",
        "[DJMantra],one_deck": "0",
        "[DJMantra],d1_hotcue": "1", "[DJMantra],d1_loop": "0",
        "[DJMantra],d1_fx": "0", "[DJMantra],d1_sampler": "0",
        "[DJMantra],d2_hotcue": "1", "[DJMantra],d2_loop": "0",
        "[DJMantra],d2_fx": "0", "[DJMantra],d2_sampler": "0",
    }
    # Remaining time, as in djay (a tap on the time changes it until the next start)
    persist = {"[Controls],ShowDurationRemaining": "1"}
    # kept between starts: One Deck's continuous play / stop after current
    kept = {"[DJMantra],one_deck_continuous": "1"}
    attr_xml = "\n".join('      <attribute config_key="%s" persist="true">%s</attribute>' % kv
                         for kv in kept.items()) + "\n" + \
        "\n".join('      <attribute config_key="%s">%s</attribute>' % kv
                         for kv in persist.items()) + "\n" + "\n".join('      <attribute config_key="%s">%s</attribute>' % kv
                         for kv in attrs.items())
    root = stack([
        (None, size_aware(performance_portrait(), performance_landscape()), None),
        ("[DJMantra],show_library", vbox(library_view(), policy="me,me"), 0),
        ("[DJMantra],waveform_fullscreen", vbox(fullscreen_waveforms(), policy="me,me"), 0),
        ("[DJMantra],one_deck", vbox(one_deck_view(), policy="me,me"), 0),
    ])
    return """<!--
  DJ Mantra skin: djay's phone layout with album art instead of turntables.
  Generated by tools/skin/gen_djmantra_skin.py: edit that, not this file.
-->
<skin>
  <manifest>
    <title>DJ Mantra</title>
    <author>DJ Mantra</author>
    <version>0.5.0</version>
    <description>Phone skin for DJ Mantra: two decks with album art, mixer, waveforms and pads, in portrait and landscape.</description>
    <language>en</language>
    <license>GPL-2.0-or-later</license>
    <attributes>
%s
    </attributes>
  </manifest>
  <ObjectName>Mixxx</ObjectName>
  <Style src="skins:DJMantra/style.qss"/>
  <MinimumSize>320,320</MinimumSize>
  <SizePolicy>me,me</SizePolicy>
  <Layout>vertical</Layout>
  <LaunchImageStyle>
    LaunchImage { background-color: %s; }
    QLabel { image: url(skins:DJMantra/svg/app_icon.svg); min-width: 128px; min-height: 128px;
             max-width: 128px; max-height: 128px; }
    QProgressBar { background-color: #333; border: none; min-width: 128px; max-width: 128px;
                   min-height: 0px; max-height: 0px; }
    QProgressBar::chunk { background-color: %s; }
  </LaunchImageStyle>
  <Children>
%s
  </Children>
</skin>
""" % (attr_xml, TOPBAR, BLUE, root)


def style_qss():
    return """/* DJ Mantra skin. Generated by tools/skin/gen_djmantra_skin.py */
#Mixxx, WWidgetStack, #Landscape, #Portrait { background-color: %(BG)s; }
WWidget, WLabel, QLabel { color: %(WHITE)s; font-family: "Roboto", "Open Sans", sans-serif; }
#TopBar { background-color: %(TOPBAR)s; padding: 6px 10px 4px 10px; }
#Centre, #Mixer { background-color: %(CENTER)s; }
#Centre { border-left: 1px solid #3a3a3e; border-right: 1px solid #3a3a3e; }
#Deck { background-color: %(PANEL)s; padding: 6px; }
#TabRow { background-color: %(CENTER)s; padding: 4px; border-bottom: 1px solid #3a3a3e; }
#Transport { background-color: %(PANEL)s; padding: 8px 10px; border-top: 1px solid #3a3a3e; }
#Selector { background-color: %(PANEL)s; border-top: 1px solid #3a3a3e;
            border-bottom: 1px solid #3a3a3e; }
#WaveformArea { background-color: #000000; }
#LoopRow { background-color: #2e2e31; padding: 2px; }
#PadsArea { background-color: %(CENTER)s; padding: 8px; }
#FullWaveforms { background-color: #000000; }

#ArtistText { color: %(GREY)s; font-size: 12px; }
#TitleText { color: %(WHITE)s; font-size: 16px; }
#TimeText { color: %(WHITE)s; font-size: 18px; }
#KeyDeck1 { color: %(GREEN)s; font-size: 13px; }
#KeyDeck2 { color: %(MAGENTA)s; font-size: 13px; }
#BpmText { color: %(WHITE)s; font-size: 20px; }
#BpmCaption, #KnobLabel { color: %(GREY)s; font-size: 11px; font-weight: bold; }
#KnobLabel { qproperty-alignment: AlignCenter; }
#PadsTitle { color: %(GREY)s; font-size: 11px; }
#LoopSize { color: %(WHITE)s; font-size: 14px; }

/* Text buttons as in djay Pro: a shade lighter than the panel, a near-black
   outline, bold white text */
WPushButton {
  color: %(WHITE)s; background-color: %(BUTTON)s; border: 2px solid %(OUTLINE)s;
  border-radius: 8px; font-size: 16px; font-weight: bold;
}
WPushButton[pressed="true"] { background-color: %(PRESSED)s; }
#SyncButton[displayValue="1"] { color: #0d0d0f; background-color: %(BLUE)s; border-color: %(BLUE)s; }
#CueSet[displayValue="1"] { border-color: #ffffff; }
#CuePair WPushButton { border-radius: 0px; }
#CueSet { border-top-left-radius: 8px; border-bottom-left-radius: 8px; border-right: 1px solid #4a4a4f; }
#CueReturn { border-top-right-radius: 8px; border-bottom-right-radius: 8px; border-left: none; font-size: 20px; }
#RoundButton, #Chevron { border-radius: 20px; font-size: 18px; }
#Chevron { border-radius: 17px; background-color: #4a4a4f; border: none; }
#RoundButton[displayValue="1"] { border-color: %(BLUE)s; color: %(BLUE)s; }
#BendButton, #IconButton { background-color: transparent; border: none; color: %(GREY)s; font-size: 22px; }
#LoopButton { background-color: transparent; border: 2px solid #6a6a70; border-radius: 16px; padding: 3px; }
#LoopButton[displayValue="1"] { border-color: %(GREEN)s; color: %(GREEN)s; }
#TabButton, #LibraryButton, #PlayButton, #GearButton { background-color: transparent; border: none; }
#CueReturn { padding: 6px; }
#SelectorButton { background-color: transparent; border: none; font-size: 24px; color: %(WHITE)s; }
#SelectorButton[displayValue="1"] { color: %(BLUE)s; }
#BackButton, #LoadButton { font-size: 13px; }
#LoadButton { margin-left: 4px; }
#CueButton { border-radius: 8px; }
#OneDeck { background-color: %(BG)s; }
#ContinuousButton { font-size: 12px; border-radius: 8px; }
#ContinuousButton[displayValue="1"] { color: %(GREEN)s; border-color: %(GREEN)s; }
#ContinuousButton[displayValue="0"] { color: %(ORANGE)s; border-color: %(ORANGE)s; }
#OneDeckCaption { color: %(GREY)s; font-size: 11px; font-weight: bold; }
#OneDeckTitle { color: %(WHITE)s; font-size: 18px; }
#OneDeckArtist { color: %(WHITE)s; font-size: 15px; }
#CueButton[displayValue="1"] { border-color: #5c5f62; }
#PflButton, #LoadDeckButton { font-size: 13px; border-radius: 6px; padding: 0px; }
#PflButton[displayValue="1"] { color: #0d0d0f; background-color: %(ORANGE)s; border-color: %(ORANGE)s; }
#ModeRow { padding: 0px 2px; }
#ModeButton { font-size: 11px; border-radius: 5px; margin: 0px 2px; border-width: 1px; }
#ModeButton[displayValue="1"] { color: #0d0d0f; background-color: #ffffff; border-color: #ffffff; }
#PadPlay { font-size: 17px; }
#PadPlay[displayValue="1"] { color: %(GREEN)s; border-color: %(GREEN)s; }
WPushButton#PadButton { background-color: #2a2d2f; border: 2px solid %(OUTLINE)s; border-radius: 8px;
                        font-size: 15px; margin: 3px; }
WPushButton#PadButton[displayValue="1"] { background-color: %(BLUE)s; border-color: %(BLUE)s; }
WHotcueButton#Pad { background-color: #2a2d2f; border: 2px solid %(OUTLINE)s; border-radius: 8px;
                    font-size: 15px; margin: 3px; }

/* Library */
#LibraryView, #LibraryTop { background-color: %(TOPBAR)s; }
#LibraryTop { padding: 6px; }
WLibrary, WLibrarySidebar, QTableView, QTreeView {
  background-color: %(BG)s; color: %(WHITE)s; border: none; font-size: 15px;
  selection-background-color: %(BLUE)s; selection-color: #ffffff;
  alternate-background-color: #28282b;
}
QHeaderView::section { background-color: %(PANEL)s; color: %(GREY)s; border: none; padding: 4px; }
WSearchLineEdit { background-color: #1b1b1d; color: %(WHITE)s; border: 1px solid #4a4a4f;
                  border-radius: 8px; padding: 4px 8px; font-size: 15px; min-height: 28px; }
QScrollBar:vertical { background: %(BG)s; width: 8px; }
QScrollBar::handle:vertical { background: #55555a; border-radius: 4px; min-height: 30px; }
QScrollBar::add-line, QScrollBar::sub-line { height: 0px; width: 0px; }
QMenu { background-color: %(PANEL)s; color: %(WHITE)s; border: 1px solid #4a4a4f; font-size: 15px; }
QMenu::item:selected { background-color: %(BLUE)s; }
QToolTip { background-color: %(PANEL)s; color: %(WHITE)s; border: 1px solid #4a4a4f; }
""" % dict(BG=BG, PANEL=PANEL, TOPBAR=TOPBAR, CENTER=CENTER, GREY=GREY, WHITE=WHITE,
           BLUE=BLUE, GREEN=GREEN, MAGENTA=MAGENTA, ORANGE="#E8A33D",
           BUTTON=BUTTON, OUTLINE=OUTLINE, PRESSED=PRESSED)


def svg(w, h, body):
    return ('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" '
            'viewBox="0 0 %d %d">%s</svg>\n' % (w, h, w, h, body))


def graphics():
    g = {}
    # knobs: a black disc in a dark ring, a white rounded line (djay Pro)
    g["knob_bg.svg"] = svg(64, 64,
        '<circle cx="32" cy="32" r="21" fill="#0f1011" stroke="#060707" stroke-width="2"/>')
    g["knob_indicator.svg"] = svg(64, 64,
        '<rect x="30" y="13.5" width="4" height="13" rx="2" fill="#ffffff"/>')
    ticks = "".join('<rect x="2" y="%d" width="40" height="1" fill="#3e3e43"/>' % y
                    for y in range(20, 300, 56))
    g["fader_track.svg"] = svg(44, 300, ticks +
        '<rect x="19" y="6" width="6" height="288" rx="3" fill="#3a3a3e"/>')
    # fader caps: grey pills with a soft gradient (djay Pro)
    pill = ('<defs><linearGradient id="g" x1="0" y1="0" x2="0" y2="1">'
            '<stop offset="0" stop-color="#a3a6a9"/><stop offset="1" stop-color="#6f7275"/>'
            '</linearGradient></defs>')
    g["fader_handle.svg"] = svg(44, 22, pill +
        '<rect x="1" y="2" width="42" height="18" rx="9" fill="url(#g)"/>')
    g["pitch_track.svg"] = svg(44, 300, ticks +
        '<rect x="20" y="6" width="4" height="288" rx="2" fill="#3a3a3e"/>'
        '<rect x="4" y="149" width="36" height="2" fill="#6a6a70"/>')
    g["pitch_handle.svg"] = svg(44, 22, pill +
        '<rect x="1" y="2" width="42" height="18" rx="9" fill="url(#g)"/>')
    xticks = "".join('<rect x="%d" y="6" width="1.5" height="36" fill="#4a4a4f"/>' % x
                     for x in range(20, 400, 60))
    g["xfader_track.svg"] = svg(400, 48, xticks +
        '<rect x="6" y="22" width="388" height="4" rx="2" fill="#3a3a3e"/>')
    g["xfader_handle.svg"] = svg(22, 48,
        '<defs><linearGradient id="h" x1="0" y1="0" x2="1" y2="0">'
        '<stop offset="0" stop-color="#a3a6a9"/><stop offset="1" stop-color="#6f7275"/>'
        '</linearGradient></defs>'
        '<rect x="2" y="1" width="18" height="46" rx="9" fill="url(#h)"/>')
    segs_off = "".join('<rect x="0" y="%d" width="12" height="4" fill="#4a4a50"/>' % y
                       for y in range(0, 300, 6))

    def seg_color(y):
        return "#ff3030" if y < 30 else ("#ffd400" if y < 72 else GREEN)
    segs_on = "".join('<rect x="0" y="%d" width="12" height="4" fill="%s"/>' % (y, seg_color(y))
                      for y in range(0, 300, 6))
    g["vu_back.svg"] = svg(12, 300, segs_off)
    g["vu_on.svg"] = svg(12, 300, segs_on)
    # play: a solid white triangle in a rounded rectangle; playing: pause
    def transport_button(body, fill):
        return svg(64, 64, '<rect x="2" y="8" width="60" height="48" rx="9" fill="%s" '
                   'stroke="%s" stroke-width="2.5"/>' % (fill, OUTLINE) + body)
    tri = ('<path d="M26 21.5 L44 32 L26 42.5 Z" fill="#ffffff" stroke="#ffffff" '
           'stroke-width="2" stroke-linejoin="round"/>')
    bars = ('<rect x="24" y="22" width="6" height="20" rx="1.5" fill="#ffffff"/>'
            '<rect x="34" y="22" width="6" height="20" rx="1.5" fill="#ffffff"/>')
    g["play_off.svg"] = transport_button(tri, BUTTON)
    g["play_on.svg"] = transport_button(bars, BUTTON)
    g["play_pressed.svg"] = transport_button(tri, PRESSED)
    # the menu button and the app's "dj" mark (after Marko's model, changed a
    # little): a heavy lowercase "dj", a round bowl, a tall d stem, a j with a
    # round dot and a hook under the d, white on a black rounded tile
    g["dj_logo.svg"] = svg(40, 40,
        '<rect x="0.5" y="0.5" width="39" height="39" rx="9" fill="#000000" '
        'stroke="#2e3133" stroke-width="1"/>'
        '<g transform="translate(2,-1)" fill="none" stroke="#ffffff" stroke-width="4.2" '
        'stroke-linecap="round" stroke-linejoin="round">'
        '<circle cx="13.6" cy="24.6" r="5.4"/>'
        '<path d="M19.4 9.6v20.9"/>'
        '<path d="M28 19.4v12.4a4 4 0 0 1-4 4h-0.6"/></g>'
        '<circle cx="30" cy="12.6" r="2.5" fill="#ffffff"/>')
    # the start screen: the app's icon (tools/android/gen_launcher_icon.py)
    import importlib.util
    spec = importlib.util.spec_from_file_location(
        "gen_launcher_icon", os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                          "..", "android", "gen_launcher_icon.py"))
    icon = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(icon)
    g["app_icon.svg"] = icon.svg_icon()
    g["prev.svg"] = svg(48, 48, '<rect x="11" y="14" width="4" height="20" rx="1" fill="#ffffff"/>'
                        '<path d="M37 14 L17 24 L37 34 Z" fill="#ffffff"/>')
    g["next.svg"] = svg(48, 48, '<rect x="33" y="14" width="4" height="20" rx="1" fill="#ffffff"/>'
                        '<path d="M11 14 L31 24 L11 34 Z" fill="#ffffff"/>')
    g["record.svg"] = svg(64, 64,
        '<circle cx="32" cy="32" r="27" fill="#0d0d0f" stroke="#ffffff" stroke-width="4"/>'
        '<circle cx="32" cy="32" r="9" fill="#ffb020"/>')
    # no album art: djay's beamed notes in a dashed rounded square
    g["cover_default.svg"] = svg(200, 200,
        '<rect width="200" height="200" fill="#1e2021"/>'
        '<rect x="6" y="6" width="188" height="188" rx="22" fill="none" stroke="#5c5f62" '
        'stroke-width="4" stroke-dasharray="11 9"/>'
        '<path d="M80 72 L132 62 L132 76 L80 86 Z" fill="#8a8d90"/>'
        '<rect x="80" y="72" width="7" height="58" fill="#8a8d90"/>'
        '<rect x="125" y="62" width="7" height="58" fill="#8a8d90"/>'
        '<ellipse cx="74" cy="131" rx="13" ry="10" transform="rotate(-18 74 131)" fill="#8a8d90"/>'
        '<ellipse cx="119" cy="121" rx="13" ry="10" transform="rotate(-18 119 121)" fill="#8a8d90"/>')

    # back to the cue point: djay's arc arrow over a dot
    g["cue_return.svg"] = svg(40, 40,
        '<path d="M9 26 C10 17 22 12.5 28.5 18.5" fill="none" stroke="#ffffff" '
        'stroke-width="3.4" stroke-linecap="round"/>'
        '<path d="M24.6 13.4 L32.6 20.6 L23.4 22.4 Z" fill="#ffffff" stroke="#ffffff" '
        'stroke-width="1.2" stroke-linejoin="round"/>'
        '<circle cx="31.5" cy="29" r="2.7" fill="#ffffff"/>')
    def loop_icon(color):
        return svg(52, 34,
            '<path d="M14 13 A9 9 0 0 1 36 11" fill="none" stroke="%s" stroke-width="3"'
            ' stroke-linecap="round"/><path d="M33 5 L39 12 L31 14 Z" fill="%s"/>'
            '<path d="M38 21 A9 9 0 0 1 16 23" fill="none" stroke="%s" stroke-width="3"'
            ' stroke-linecap="round"/><path d="M19 29 L13 22 L21 20 Z" fill="%s"/>'
            % ((color,) * 4))
    g["loop.svg"] = loop_icon("#d8d8dc")
    g["loop_on.svg"] = loop_icon(GREEN)
    g["chevron.svg"] = svg(34, 34,
        '<circle cx="17" cy="17" r="16" fill="#4a4a4f"/>'
        '<path d="M10 14 L17 21 L24 14" fill="none" stroke="#ffffff" stroke-width="3"'
        ' stroke-linecap="round" stroke-linejoin="round"/>')
    teeth = "".join('<rect x="16.5" y="3" width="5" height="8" rx="1.5" fill="#d8d8dc" '
                    'transform="rotate(%d 19 19)"/>' % a for a in range(0, 360, 45))
    g["gear.svg"] = svg(38, 38, teeth +
        '<circle cx="19" cy="19" r="10" fill="#d8d8dc"/><circle cx="19" cy="19" r="4.5" fill="#0d0d0f"/>')

    def tab(icon_body, on):
        bg = '<rect x="6" y="1" width="48" height="32" rx="6" fill="%s"/>' % BLUE if on else ""
        return svg(60, 34, bg + icon_body)
    mixer_icon = ('<rect x="20" y="8" width="3" height="18" fill="#fff"/>'
                  '<rect x="16" y="12" width="11" height="4" rx="1" fill="#fff"/>'
                  + "".join('<rect x="30" y="%d" width="12" height="3" fill="#fff"/>' % y
                            for y in range(8, 27, 5)))
    wave_icon = "".join('<rect x="%d" y="%d" width="2" height="%d" fill="#fff"/>'
                        % (16 + i * 3, 17 - h // 2, h)
                        for i, h in enumerate([4, 8, 14, 20, 12, 18, 8, 4, 2]))
    grid_icon = "".join('<rect x="%d" y="%d" width="6" height="6" fill="#fff"/>' % (20 + c * 8, 6 + r * 8)
                        for r in range(3) for c in range(3))
    for name, body in (("tab_mixer", mixer_icon), ("tab_wave", wave_icon), ("tab_pads", grid_icon)):
        g[name + ".svg"] = tab(body, False)
        g[name + "_on.svg"] = tab(body, True)
    return g


def main():
    if os.path.isdir(SKIN):
        shutil.rmtree(SKIN)
    os.makedirs(os.path.join(SKIN, "svg"))
    with open(os.path.join(SKIN, "skin.xml"), "w") as f:
        f.write(skin_xml())
    with open(os.path.join(SKIN, "style.qss"), "w") as f:
        f.write(style_qss())
    for name, content in graphics().items():
        with open(os.path.join(SKIN, "svg", name), "w") as f:
            f.write(content)
    print("wrote", SKIN)


if __name__ == "__main__":
    main()
