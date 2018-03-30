# MQTT-Security-Panel
I replaced my old 1990s security system with a more modern system. My home is already running Home Assistant with an MQTT server. 

This security panel is built on an Arduino Mega 2560, Ethernet Shield, and a Screw Proto Shield. It's powered with POE, and my switch is on a UPS, so this does have a battery backup. 

# Software
You'll need a few 3rd party Arduino libraries:
* MQTT Client: [PubSubClient](https://pubsubclient.knolleary.net/)
* [Simple Arduino Task Scheduler](https://github.com/ryannazaretian/Simple-Arduino-Task-Scheduler)

# Hardware (from Amazon)
First off, I'm be believer in the Open-Source hardware. Please buy at least one official Arduino board to support the cause and development. With that said, this is exactly the hardware I used. 
* [Elegoo ATmega2560](https://www.amazon.com/gp/product/B01H4ZDYCE/ref=oh_aui_detailpage_o04_s00?ie=UTF8&psc=1)
* [WINGONEER Prototype Screw / Terminal Block Shield](https://www.amazon.com/gp/product/B01N2N7LZA/ref=oh_aui_detailpage_o04_s01?ie=UTF8&psc=1)
* [SunFounder Ethernet Shield W5100](https://www.amazon.com/gp/product/B00HG82V1A/ref=oh_aui_detailpage_o04_s01?ie=UTF8&psc=1)
* [WiFi-Texas POE Splitter - 5V, 2A](https://www.amazon.com/gp/product/B019BLMWY0/ref=oh_aui_detailpage_o04_s00?ie=UTF8&psc=1)
* [Terminal Strip (or something like this)](https://www.amazon.com/Terminals-Electric-Barrier-12-Position-Terminal/dp/B01N3AJOYK/ref=pd_sbs_328_4?_encoding=UTF8&pd_rd_i=B01N3AJOYK&pd_rd_r=YB7HMNVXDCC283ZZ8Z9P&pd_rd_w=Kjnwy&pd_rd_wg=WD4IZ&psc=1&refRID=YB7HMNVXDCC283ZZ8Z9P)
* Status LED (sorry, no link here. I had a bi-color LED in my stock. Use it or leave it. You could also use the built in D13 status LED.)

# Installation
Here's my installation. It's not pretty, and I do plan to clean it up at some point. 

![Installed Security Panel](https://github.com/ryannazaretian/MQTT-Security-Panel/blob/master/images/installed.jpg?raw=true)

