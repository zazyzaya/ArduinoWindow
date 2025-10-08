# Synthetic Window

<div style="
  width:80%;
  margin: 1.5em auto;
  background: #fdfdfd;
  border-left: 5px solid #4a90e2;
  box-shadow: 0 2px 8px rgba(0,0,0,0.08);
  border-radius: 6px;
  padding: 1em 1.5em;
  font-family: 'Georgia', serif;
  color: #333;
  line-height: 1.6;
">

  <p style="font-size:1.05em; margin-top:0; margin-bottom:0.8em;">
    <em>“Workers in windowless environments reported poorer scores than <strong>[those with windows]</strong> ... due to physical problems and vitality — as well as poorer overall sleep quality.”</em>
  </p>

  <hr style="border:none; border-top:1px solid #ddd; margin:0.8em 0;">

  <p style="font-size:0.95em; margin:0; text-align:right; color:#555;">
    <a href="https://pmc.ncbi.nlm.nih.gov/articles/PMC4031400/" style="color:#2a5db0; text-decoration:none; font-weight:bold;">
      Impact of Windows and Daylight Exposure on Overall Health and Sleep Quality of Office Workers: A Case-Control Pilot Study
    </a><br>
    <span style="font-style:italic;">Boubekri et al., Journal of Sleep Medicine (2014)</span>
  </p>

</div>

I have lived in an apartment with no windows for almost three years now. It is driving me insane. My circadian rhythm is all out of whack. 

## Code 
Add a `secrets.h` file containing

    #define LAT [your latitude]
    #define LON [your longitude]

and upload to your preferred microcontroller. (Code assumes you are using an Arduino Nano Every)

Tracks the position of the sun and changes the colors of the LEDs to match below: 

<div align="center">
  <img src="img/led_colors.png" width="400">
  <h5>Figure 1. LED color cycle</h5>
</div>

Wire everything up according to the following circuit diagram and your apartment will finally have a window: 

## Wiring 

<div align="center">

<table>
  <tr>
    <td align="center"><img src="img/schematic.png" width="300"></td>
    <td align="center"><img src="img/proto_board.jpeg" width="300"></td>
  </tr>
  <tr>
    <td align="center">a.) Optimistic Schematic</td>
    <td align="center">b.) Actual Spaghetti Board</td>
  </tr>
</table>

<h5>Figure 2. Wiring Diagram vs Reality</h5>

</div>

**Parts list**: 

* Arduino Nano Every
* HiLetGo DS3231 RTC 
* Cheap Amazon WS2811 LEDs (100x)
* 5V, 50W power supply 

Connect the RTC's SDA pin to the Arduino A4 pin, and the SCL pin to the A5. Connect the data pin on the lights to the D2 pin. Wire the positive terminals on the lights and RTC to the positive power supply, and run a jumper from the Arduino VIN pin to the same. Then ground everything to the input ground. 

## Woodworking 

(In progress)

The actual window mounting is two components: a shadowbox to hold the electronics, and mount the LEDs to, and a window to cover everything up. The shadowbox is complete (see below), and the woodworking sketches for the window frame are also done, but the materials for the window bit are still up in the air. 

Initially, I was going to use the plastic panels that cover LED lights in office buildings, but they all look very "crystal" and unnatural. I'm now thinking I'll get a peice of thin plastic sheeting (maybe the [plastic used in wire shelves](https://www.webstaurantstore.com/regency-shelving-clear-pvc-shelf-mat-overlay-24-x-48/460MATCL2448.html?utm_source=google&utm_medium=cpc&gbraid=0AAAAAD_Dx-vHoIGIAyfE91Kf8GcR_R8S5&gclid=Cj0KCQjw9JLHBhC-ARIsAK4PhcoE0dyCP0kGweBKh7jFsD5pUX2WQJBRpYF8jP98X-JUJFus_wWyagsaAsFpEALw_wcB)?) and place that over a linen sheet to diffuse the light. But we shall see. 

For now, enjoy the shadowbox instructions below.

### Shadowbox 

<div align="center">

<table>
  <tr>
    <td align="center"><img src="img/inner_frame_schamatic.jpeg" width="300"></td>
    <td align="center"><img src="img/inner_frame.jpeg" width="300"></td>
  </tr>
  <tr>
    <td align="center">a.) Sketch</td>
    <td align="center">b.) Reality</td>
  </tr>
</table>

<h5>Figure 3. Shadowbox component holder</h5>
</div>

**Parts List** 

* Fiberboard (Sold in 2'x4', cut to 2'x3')
* 1"x2" furring: 
  * 2x 7" 
  * 2x 36"
  * 1x 22" 

You can ask the guy at Home Depot to cut the 8' furring planks into these little peices for you. They won't be happy about it, but they'll do it. 

In hindsight, the little hole for easy access to the electronics was larger than necessary. The PSU fits nicely on the little shelf at the bottom. I was worried I'd need to mount it an extra inch lower, but this ended up being unneccesary. 

<span style="color:red;">TODO</span>: cover the bottom bit with some rubber or something to make a dust cover. 

<span style="color:red;">TODO</span>: punch some nails through the back to hang the LEDs from. Hot glue the leds to the nails once they're in place. 