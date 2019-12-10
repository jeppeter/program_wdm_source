// D:\WdmbookWhistler\chap13\hidfake\sys\ReportDescriptor.h


char ReportDescriptor[64] = {
    0x05, 0x05,                    // USAGE_PAGE (Gaming Controls)
    0x09, 0x03,                    // USAGE (Gun Device )
    0xa1, 0x01,                    // COLLECTION (Application)
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x01,                    //     REPORT_ID (1)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x01,                    //     USAGE (Button 1)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x75, 0x07,                    //     REPORT_SIZE (7)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x02,                    //     REPORT_ID (2)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x25, 0xff,                    //     LOGICAL_MAXIMUM (-1)
    0x75, 0x20,                    //     REPORT_SIZE (32)
    0xb1, 0x02,                    //     FEATURE (Data,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xa1, 0x02,                    //   COLLECTION (Logical)
    0x85, 0x03,                    //     REPORT_ID (3)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x09, 0x01,                    //     USAGE (Button 1)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0xb1, 0x02,                    //     FEATURE (Data,Var,Abs)
    0x75, 0x07,                    //     REPORT_SIZE (7)
    0xb1, 0x03,                    //     FEATURE (Cnst,Var,Abs)
    0xc0,                          //   END_COLLECTION
    0xc0                           // END_COLLECTION
};

