import sys
import csv


def saveIP(ip):
    if ip:
        if ip == "" or ip == "0x0" or ip == "NULL":
            return False

        ip = removeComents(ip)
        ip = ip.strip()

    else:
        return False
    return ip


def saveCompany(company):
    if company:
        if company == "" or company == "0x0" or company == "NULL":
            return "NULL"

        company = removeComents(company)
        company = company.strip()

        # if flag
        if company.find("NDPI") != -1:
            return "NULL"

        else:
            return company
    else:
        return "NULL"


def removeComents(data):
    if data.find("/*") != -1:
        if data.find("*/") != -1:
            data = data.split("/*")[0] + data.split("*/")[1]
    return data


def removeElement(data):
    # remove first element from list
    if len(data) > 0:
        data = data[1:]
    return data


# load SNI file
if len(sys.argv) > 1:
    print("First argument:", sys.argv[1])
else:
    print("No arguments given. Provide path to SNI file")
    exit()

sni_file_path = sys.argv[1]

csv_file = open("sni.csv", "w")
csv_file.write('"IP/Domain","Company","Flags"\n')

sni_file = open(sni_file_path, "r")
sni_lines = sni_file.readlines()


for line in sni_lines:
    # remove whitespace and check if line starts with {
    line = line.strip()
    if line.startswith("{"):
        # remove {
        line = line[1:-1]
        # remove everything after }
        if line.find("}") != -1:
            line = line.split("}")[0]
        # split by comma
        reader = csv.reader([line], skipinitialspace=True)
        line = next(reader)
        line = [x.strip() for x in line]

        # line = line.split(",")
        # # remove whitespace from each element
        # line = [x.strip() for x in line]

        # handle each element

        # save ip/domain
        ip = "NULL"
        if len(line) > 0:
            ip = saveIP(line[0])
        else:
            continue

        if not ip:
            continue
        line = removeElement(line)

        company = "NULL"
        flags = []

        if len(line) > 0:
            company = saveCompany(line[0])
        else:
            continue

        if company != "NULL":
            line = removeElement(line)

        if company.isnumeric():
            company = "NULL"

        # save flags
        if len(line) > 0:
            for flag in line[0:]:
                flags.append(flag)
        else:
            continue

        # join flags with ;
        flags = ";".join(flags)

        # put "" around all fields and create csv line
        line = f'"{ip}","{company}","{flags}"'

        # push the line to new csv file
        csv_file.write(line + "\n")


sni_file.close()
