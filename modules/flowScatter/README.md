# flowScatter module - README

## Description
The module performs distributing of 1 unirec interface data to `n` trap outputs

## Interfaces
- Input: 1
- Output: `n`

## Parameters
### Common TRAP parameters
- `-h [trap,1]`      Print help message for this module / for libtrap specific parameters.
- `-i IFC_SPEC`      Specification of interface types and their parameters.
- `-v`               Be verbose.
- `-vv`              Be more verbose.
- `-vvv`             Be even more verbose.

### Module specific parameters
- `-m, --appfs-mountpoint <path>` Path where the appFs directory will be mounted
- `-r, --rule <string>` Decide, what fields are used to create a hash. Might be in form of rule.
- `-c, --count <int>` Number of output interfaces.

## Usage Examples
```
$ flowscatter -i u:trap_in,u:trap_out -r "SRC_IP,SRC_PORT" # according SRC_IP+SRC_PORT

$ flowscatter -i u:trap_in,u:trap_out -r "TAG_SRC_IP:(SRC_IP)|TAG_DST_IP:(DST_IP)" # if TAG_SRC_IP is not 0, then SRC_IP else if TAG_DST_IP is no 0, then DST_IP

$ flowscatter -i u:trap_in,u:trap_out -r "TAG_SRC_IP:(SRC_IP,SRC_PORT)|TAG_DST_IP:(DST_IP,DST_PORT)" # if TAG_SRC_IP is not 0, then SRC_IP+SRC_PORT, else if TAG_DST_IP is not 0, then DST_IP+DST_PORT

```
