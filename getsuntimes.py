#!/usr/bin/env python3

# %%
import datetime
from time import time
from astral.sun import sun
from astral import LocationInfo
import argparse
# %%
parser = argparse.ArgumentParser()
parser.add_argument('--type', type=str, required=True)
parser.add_argument('--lat', type=float, required=False)
parser.add_argument('--lon', type=float, required=False)
# %%
args = parser.parse_args()
# %%
if args.lat is not None:
    lat = args.lat
else:
    lat = 42.6334
if args.lon is not None:
    lon = args.lon
else:
    lon = -71.3162
# %%
loc = LocationInfo(timezone='UTC', latitude=lat, longitude=lon)
s = sun(loc.observer, date = datetime.datetime.utcnow().date(), tzinfo = loc.timezone)
sunrise = int((s['sunrise']+datetime.timedelta(minutes = 10)).timestamp()*1000)
sunset = int((s['sunset']-datetime.timedelta(minutes = 10)).timestamp()*1000)
now = int((datetime.datetime.utcnow()+datetime.timedelta(hours = 7)).timestamp()*1000)

if (sunrise <= now < sunset): # daytime, find next sunrise
    s = sun(loc.observer, date = (datetime.datetime.utcnow()+datetime.timedelta(hours = 24)).date(), tzinfo = loc.timezone)
    sunrise = int((s['sunrise']+datetime.timedelta(minutes = 10)).timestamp()*1000)
elif (sunrise < sunset < now):
    s = sun(loc.observer, date = (datetime.datetime.utcnow()+datetime.timedelta(hours = 12)).date(), tzinfo = loc.timezone)
    sunrise = int((s['sunrise']+datetime.timedelta(minutes = 10)).timestamp()*1000)

# %%
if args.type == 'sunrise':
    print(sunrise) # in ms
elif args.type == 'sunset':
    print(sunset) # in ms
else:
    print(0)
