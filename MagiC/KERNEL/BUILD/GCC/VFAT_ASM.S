     INCLUDE "VFAT_S.S"

; Keep these in agreement with the GCC compilation unit's layout assertions.
     IFNE fd_dev-$08
     FAIL "FD device offset differs from C"
     ENDIF
     IFNE fd_fpos-$34
     FAIL "FD file-position offset differs from C"
     ENDIF
     IFNE fd_longname-$50
     FAIL "FD long-name offset differs from C"
     ENDIF
     IFNE dir_xdata-$0e
     FAIL "Directory data offset differs from C"
     ENDIF
     IFNE dir_flen-$1c
     FAIL "Directory length offset differs from C"
     ENDIF
     IFNE dir_sizeof-32
     FAIL "Directory entries must be 32 bytes"
     ENDIF
