# Geolite

## Module description

This module outputs flow records with geolocation data using a [geolite database](https://dev.maxmind.com/geoip/geolite2-free-geolocation-data/).

## Input data

This module expects flow records in Unirec format. The required fields
are determined by run time parameters.

## Output data

Flows are sent on the output interface, also in Unirec format. Additional fields are available

* Geolite module:
    * string `CITY_NAME`
    * string `COUNTRY_NAME`
    * double `LATITUDE`
    * double `LONGITUDE`
    * string `POSTAL_CODE`
    * string `CONTINENT_NAME`
    * string `ISO_CODE`
    * uint16 `ACCURACY`
* ASN module:
    * string `ASO`
    * uint32 `ASN`

Each field come in two variants, one for source IP address with prefix `SRC_` and one for
destination IP address with prefix `DST_`. For example, the field for source IP address `city_name` is
`SRC_CITY_NAME` and for destination IP address it is `DST_CITY_NAME`.

SRC and DSR fields are added based on `-t` parameter value.

## Module parameters

In addition to the implicit *libtrap* parameters `-i IFC_SPEC`, `-h`
and `-v` (see [Execute a
module](https://github.com/CESNET/Nemea#try-out-nemea-modules)) this
module takes the following parameters:

* `-f` `--fields` field1,field2,field3,...

  * List of fields from plugins that will be added to the output records. (default is all fields)

    * Geolite module fields are: `city_name`, `country_name`, `latitude`, `longitude`, `postal_code`
      `continent_name`, `iso_code`, `accuracy`.

    * ASN module fields are: `asn`, `aso`.

  * Do NOT insert spaces between fields, use comma ',' as a separator.

  * Name of the field is case INSENSITIVE, fields are ALWAYS exported in UPPER CASE and with prefix
    `SRC_` or `DST_` depending on the traffic direction. E.g. `city_name` will be exported as Unirec
    field/s `SRC_CITY_NAME` or/and `DST_CITY_NAME`.

*  `--pathGeolite` path

  * Specify path to the MaxMind GeoLite City file (.mmdb).

*  `--pathASN` path

  * Specify path to the MaxMind GeoLite ASN file (.mmdb).

* `-s` `--source` field

  * Specify the name of field with source IP, which will be used for geolocation and lookup in the database (case sensitive). Default is `SRC_IP`.

* `-d` `--destination` field

  * Specify the name of field with destination IP, which will be used for geolocation and lookup in the database (case sensitive). Default is `DST_IP`.

* `-t` `--traffic-direction` field

  * Specify if the geolocation should be done for source, destination or both IP addresses. Possible
    values are `src`, `dst` or `both`. Default is `both`.

## Example
The following command :

`./geolite -i f:/etc/nemea/data/data.dan.trapcap,f:test.trapcap -pathGeolite '/usr/share/GeoIP/GeoLite2-City.mmdb'`

will be interpreted as follows:

* `-i f:/etc/nemea/data/data.dan.trapcap,f:test.trapcap`
  sets the input and output interfaces to a file.

* `-pathGeolite '/usr/share/GeoIP/GeoLite2-City.mmdb'` sets the path to the database file.
