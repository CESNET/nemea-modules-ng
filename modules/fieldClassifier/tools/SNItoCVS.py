###############################

# Convert SNI file to CSV
# Usage: python3 SNItoCVS.py <path_to_sni_file>
# Output: listIP.csv, listSNI.csv

# listIP.csv columns: IP, hexadecimal_IP, Mask, hexadecimal_mask, IP_type, flags
#   IP_type: ipv4 or ipv6

# listSNI.csv columns: SNI, Company, Flags

# Flags are separated by ;

# Source of SNI file: https://github.com/ntop/nDPI/blob/dev/src/lib/ndpi_content_match.c.inc

# Author: Tomáš Vrána
# Email: xvranat00@vutbr.cz

###############################


import sys
import csv
import ipaddress


# check if string is hexadecimal
def is_hexadecimal_1(s):
    try:
        int(s, 16)
        return True
    except ValueError:
        return False


# check if string is IP address
def is_ip(s):
    try:
        ipaddress.ip_address(s)
        return True
    except ValueError:
        return False
    except:
        return False


def removeComents(data):
    if data.find("/*") != -1:
        if data.find("*/") != -1:
            data = data.split("/*")[0] + data.split("*/")[1]
    return data


# remove the first element from list
def removeElement(data):
    if len(data) > 0:
        data = data[1:]
    return data


# load SNI file
if len(sys.argv) <= 1:
    print("No arguments given. Provide path to SNI file")
    exit()

sni_file_path = sys.argv[1]


sni_file = open(sni_file_path, "r")
sni_lines = sni_file.readlines()

csv_file_IP = open("/tmp/sniIP.csv", "w")
csv_file_IP.write("IP,IPInHex,Mask,MaskInHex,Type,Flags\n")

csv_file_TLS = open("/tmp/sniTLS.csv", "w")
csv_file_TLS.write("Domain,Company,Flags\n")

for line in sni_lines:

    # remove whitespace and check if line starts with {
    line = line.strip()
    if not line.startswith("{"):
        continue

    # remove {
    line = line[1:-1]

    # remove everything after } including
    if line.find("}") != -1:
        line = line.split("}")[0]

    # split by comma but ignore text in ""
    reader = csv.reader([line], skipinitialspace=True)
    line = next(reader)
    line = [x.strip() for x in line]

    # handle each element

    first = "NULL"
    second = "NULL"
    flags = []
    isIP = False
    typeIP = "ipv6"
    ipInHex = 0x0
    maskInHex = 0x0

    # save first element - IP or SNI
    if len(line) > 0:
        first = line[0]
    else:
        continue

    first = removeComents(first)
    first = first.strip()

    # skip empty, 0x0 and NULL
    if first == "" or first == "0x0" or first == "NULL":
        continue

    # check if IP
    if is_hexadecimal_1(first) or is_ip(first):
        # save IP
        line = removeElement(line)
        isIP = True

        # conver hex to ip
        if is_hexadecimal_1(first):
            first = str(ipaddress.ip_address(int(first, 16)))
            typeIP = "ipv4"

        # MASK
        if len(line) > 0:
            second = line[0]
            second = removeComents(second)
            second = second.strip()
            # save mask in hex
            if typeIP == "ipv4":
                net = ipaddress.IPv4Network(first + "/" + second, strict=False)
                maskInHex = hex(int(net.netmask))
                ipInHex = hex(int(ipaddress.IPv4Address(first)))

            else:
                net = ipaddress.IPv6Network(first + "/" + second, strict=False)
                maskInHex = hex(int(net.netmask))
                ipInHex = hex(int(ipaddress.IPv6Address(first)))

            # remove leading 0x
            ipInHex = ipInHex[2:]
            maskInHex = maskInHex[2:]

            line = removeElement(line)
        else:
            continue

    else:
        # TLS
        line = removeElement(line)
        isIP = False

        # Save company name if provided
        if len(line) > 0 and line[0].find("NDPI") == -1:
            second = line[0]
            second = removeComents(second)
            second = second.strip()
            line = removeElement(line)
        else:
            second = "NULL"

    # save flags
    flags = []

    if len(line) > 0:
        for flag in line[0:]:
            flags.append(flag)
    else:
        continue

    # join flags with ;
    flags = ";".join(flags)

    # push the line to new csv file
    if isIP:
        line = f"{first},{ipInHex},{second},{maskInHex},{typeIP},{flags}"
        csv_file_IP.write(line + "\n")
    else:
        line = f"{first},{second},{flags}"
        csv_file_TLS.write(line + "\n")

sni_file.close()
