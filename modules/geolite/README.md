# Geolite

## Module description

This module outputs flow records with geolocation data using a [geolite database](https://dev.maxmind.com/geoip/geolite2-free-geolocation-data/).

## Input data

This module expects flow records in Unirec format. The required fields
are determined by run time parameters.

## Output data

Flows are sent on the output interface, also in Unirec format, they
contain geolocation data with following additional fields:

* string `CITY_NAME`
* string `COUNTRY_NAME`
* double `LATITUDE`
* double `LONGITUDE`
* string `POSTAL_CODE`

Each field come in two variants, one for source IP address with prefix `SRC_` and one for
destination IP address with prefix `DST_`. For example, the field for source IP address city name is
`SRC_CITY_NAME` and for destination IP address it is `DST_CITY_NAME`.

SRC and DSR fields are added based on `-c` parameter value.

## Module parameters

In addition to the implicit *libtrap* parameters `-i IFC_SPEC`, `-h`
and `-v` (see [Execute a
module](https://github.com/CESNET/Nemea#try-out-nemea-modules)) this
module takes the following parameters:

* `-p` `--path` path

  * Specify path to the database file.

* `-s` `--source` field

  * Specify the name of field with source IP, which will be used for geolocation and lookup in the database (case sensitive). Default is `SRC_IP`.

* `-d` `--destination` field

  * Specify the name of field with destination IP, which will be used for geolocation and lookup in the database (case sensitive). Default is `DST_IP`.

* `-f` `--flow-direction` field

  * Specify if the geolocation should be done for source, destination or both IP addresses. Possible
    values are `src`, `dst` or `both`. Default is `both`.

## Example
The following command :

`./geolite -i f:/etc/nemea/data/data.dan.trapcap,f:test.trapcap -p '/usr/share/GeoIP/GeoLite2-Country.mmdb'`

will be interpreted as follows:

* `-i f:/etc/nemea/data/data.dan.trapcap,f:test.trapcap`
  sets the input and output interfaces to a file.

* `-d '/usr/share/GeoIP/GeoLite2-Country.mmdb'` sets the path to the database file.
