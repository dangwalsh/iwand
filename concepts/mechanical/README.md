# Mechanical iWand

iWand with no electronics. The user's eye is the light sensor, and optics plus a printed card do the classifying.

This works because iWand's users are colour-blind, not blind. Colour-blind people can't tell hues apart, but they can judge whether two patches are equally bright. That's the only judgement this design asks for.

Status: concept. Illustrated page: [mechanical-iwand.html](mechanical-iwand.html) (download and open it in a browser; GitHub shows it as source). Published copy: https://claude.ai/artifact/9vKL7WXXhBtQ7pwk7KAYuC (private; share it from the page's Share menu).

## How it works

Each of three channels (red, green, blue) has a split field in the eyepiece:

- One half shows the object. The other half shows a white tile inside the device.
- **Both halves pass through the same colour filter**, so the viewer's own colour vision cancels out. Even someone who sees red light as dim gets the right answer, because they're only comparing like with like.
- A dial dims the tile half. The user turns it until the dividing line disappears. The dial setting is then that channel's reflectance as a fraction of white, the same white point the firmware's calibration provides.
- Daylight through a frosted window lights both the object and the tile, so a change in light affects both halves equally.

## Parts

| Element | Choice | Why |
|---|---|---|
| Filters | Red, green and blue filters like the classic Wratten 25 / 58 / 47 set (gel or glass) | Wide bands like the photodiode channels, little overlap |
| Attenuator | Two linear polarizers: one fixed, one on a rotating dial | Cheap film, smooth from 100% to about 0.1%, only a quarter turn |
| Object half | One fixed polarizer, or a neutral filter with the same loss | White paper matches the tile at dial zero |
| White reference | Matte white tile (PTFE or barium-sulphate paint) | Lit by the same window as the object |
| Lighting | Frosted window around the nose, lighting the object at about 45° | Keeps glossy reflections out of view |
| Split edge | Small 45° mirror covering half of each viewing tube | A sharp dividing line, which the eye needs to judge a match |

Light through two polarizers falls as cos²(angle). Its markings are most finely spread near white, where the White/Gray decision needs the most precision.

## Dial markings

The dial is marked in optical density (D): 0 is as bright as the white tile, 1 is a tenth as bright, 2 a hundredth.

| D | Dial angle |
|---|---|
| 0 | 0° |
| 0.03 | 15° |
| 0.10 | 27° |
| 0.21 | 38° |
| 0.34 | 48° |
| 0.50 | 56° |
| 1.0 | 72° |
| 1.33 | 78° |
| 2.0 | 84° |

Angle = arccos(√(10^−D)).

## Reading the result

From the [shared thresholds](../README.md#converting-to-linear-light):

| Decision | Reading |
|---|---|
| Black | Brightest dial (lowest D) above 1.33 |
| Dark band (Brown, dark red/orange) | Brightest dial above 0.34 |
| White, if no hue | Brightest dial below 0.21 |
| No hue (White/Gray) | All three dials within 0.03 of each other |
| Hue sector | The order of the three dials: 6 orders = 6 sectors of 60° |

Brightness order and hue sector:

| Brightest → dimmest | Hue sector | Names |
|---|---|---|
| R > G > B | 0–60° | Red, Orange, Yellow (Brown if dark) |
| G > R > B | 60–120° | Yellow, Green |
| G > B > R | 120–180° | Green, Cyan |
| B > G > R | 180–240° | Cyan, Blue, Purple |
| B > R > G | 240–300° | Purple |
| R > B > G | 300–360° | Purple, Red |

Inside a sector, the gap between the readings picks the name. For a strongly coloured object, Orange/Yellow (40°) is where G reads about 0.39 above R, and Red/Orange (15°) about 1.33 above. For paler objects these shift, so the real lookup card will be generated from the thresholds and tested on paper samples.

### Worked example: orange paper

1. Readings: R 0.05, G 0.55, B 1.35.
2. Spread is 1.30, far above 0.03, so it has a hue.
3. Brightest is R at 0.05: bright, so not Black or Brown.
4. Order R > G > B: the 0–60° sector.
5. G is 0.50 above R, about 27°: **Orange**.

## Limits

- The White/Gray test needs matches accurate to about 0.03 D. The eye manages about 0.005–0.01 D in a split field, so it works with less margin than the other decisions. Pale pastels are the most likely to be misnamed.
- Slower than the electronic versions: about 20–30 seconds for three matches.
- Needs reasonable daylight or room light. There is no lamp.
- Very dark colours are hard to match by eye, so Black vs dark Brown is less reliable.

## Next steps

1. Bench test: filter gel and two pieces of polarizer film over a cardboard tube, against a white card. Check whether two halves can be matched to about 0.03 D.
2. Generate the lookup card from the thresholds with a script.
3. Lay out the optics with dimensions for Onshape.
4. Later: a direct-reading version where the R and B dials measure relative to the G dial and move crossed pointers over a printed map of colour names.
