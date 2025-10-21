# fieldClassifier

## Module description

This module outputs flow records with additional information. See available modules below.

## Input data

This module expects flow records in Unirec format. The required fields
are determined by run time parameters.

## Output data

Flows are sent on the output interface, also in Unirec format. Additional fields are available

* Geolite module: (MaxMind GeoLite2 City database)
    * Geolocation information about the IP address
    * string `CITY_NAME`
    * string `COUNTRY_NAME`
    * double `LATITUDE`
    * double `LONGITUDE`
    * string `POSTAL_CODE`
    * string `CONTINENT_NAME`
    * string `ISO_CODE`
    * uint16 `ACCURACY`
* ASN module: (MaxMind GeoLite2 ASN database)
    * ASN number and organization
    * string `ASO`
    * uint32 `ASN`
* IP_Classifier module:
    * Classification of IP address based on IP_Classifier database
    * string `IP_FLAGS`
* SNI_Classifier module:
    * Classification of SNI based on SNI_Classifier database
    * string `SNI_FLAGS`
    * string `COMPANY`

Each field come in two variants, one for source IP address with prefix `SRC_` and one for
destination IP address with prefix `DST_`. For example, the field for source IP address `city_name` is
`SRC_CITY_NAME` and for destination IP address it is `DST_CITY_NAME`.

SRC and DSR fields are added based on `-t` parameter value.

## Module parameters

In addition to the implicit *libtrap* parameters `-i IFC_SPEC`, `-h`
and `-v` (see [Execute a
module](https://github.com/CESNET/Nemea#try-out-nemea-modules)) this
module takes the following parameters:

## General parameters

* `-f` `--fields` field1,field2,field3,...

  * List of fields from plugins that will be added to the output records. (default is all fields)

    * Geolite module fields are: `city_name`, `country_name`, `latitude`, `longitude`, `postal_code`
      `continent_name`, `iso_code`, `accuracy`.

    * ASN module fields are: `asn`, `aso`.

    * IP_Classifier module fields are: `ip_flags`.

    * SNI_Classifier module fields are: `sni_flags`, `company`.

  * Do NOT insert spaces between fields, use comma ',' as a separator.

  * Name of the field is case INSENSITIVE, fields are ALWAYS exported in UPPER CASE and with prefix
    `SRC_` or `DST_` depending on the traffic direction. E.g. `city_name` will be exported as Unirec
    field/s `SRC_CITY_NAME` or/and `DST_CITY_NAME`.

* `-s` `--source` field

  * Specify the name of field with source IP, which will be used for geolocation and lookup in the database (case sensitive). Default is `SRC_IP`.

* `-d` `--destination` field

  * Specify the name of field with destination IP, which will be used for geolocation and lookup in the database (case sensitive). Default is `DST_IP`.

* `-t` `--traffic-direction` field

  * Specify if the geolocation should be done for source, destination or both IP addresses. Possible
    values are `src`, `dst` or `both`. Default is `both`.

## Geolite parameters

*  `--pathGeolite` path

  * Specify path to the MaxMind GeoLite City file (.mmdb). Default is `/tmp/GeoLite2-City.mmdb.`


## ASN parameters

*  `--pathASN` path

  * Specify path to the MaxMind GeoLite ASN file (.mmdb). Default is `/tmp/GeoLite2-ASN.mmdb.`

## IP_Classifier parameters

*  `--pathIP` path

  * Specify path to the IP_Classifier file (.csv). Default is `/tmp/sniIP.csv`

  * Use SNItoCSV.py script to generate the files. (See the script for more details about the file
  format.)

## SNI_Classifier parameters

*  `--pathSNI` path

  * Specify path to the SNI_Classifier file (.csv). Default is `/tmp/sniTLS.csv`

  * Use SNItoCSV.py script to generate the files. (See the script for more details about the file
  format.)

 * `--sniFieldName` field

   * Specify the name of field with SNI, which will be used for lookup in the database (case
   sensitive). Default is `TLS_SNI`.


## Example
The following command :

`./geolite -i f:/etc/nemea/data/data.dan.trapcap,f:test.trapcap -pathGeolite '/usr/share/GeoIP/GeoLite2-City.mmdb'`

will be interpreted as follows:

* `-i f:/etc/nemea/data/data.dan.trapcap,f:test.trapcap`
  sets the input and output interfaces to a file.

* `-pathGeolite '/usr/share/GeoIP/GeoLite2-City.mmdb'` sets the path to the database file.
