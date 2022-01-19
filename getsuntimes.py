#!/usr/bin/env python3

# %%
import datetime
from time import time
from astral.sun import sun
from astral import LocationInfo
import argparse
# %%
parser = argparse.ArgumentParser()
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
s0 = sun(loc.observer, date = datetime.datetime.utcnow().date(), tzinfo = loc.timezone)
sunrise0 = int((s0['sunrise']+datetime.timedelta(minutes = 10)).timestamp()*1000)
sunset0 = int((s0['sunset']-datetime.timedelta(minutes = 10)).timestamp()*1000)
s1 = sun(loc.observer, date = (datetime.datetime.utcnow() + datetime.timedelta(hours = 24)).date(), tzinfo = loc.timezone)
sunrise1 = int((s1['sunrise']+datetime.timedelta(minutes = 10)).timestamp()*1000)
sunset1 = int((s1['sunset']-datetime.timedelta(minutes = 10)).timestamp()*1000)
print(sunrise0, sunset0, sunrise1, sunset1)
