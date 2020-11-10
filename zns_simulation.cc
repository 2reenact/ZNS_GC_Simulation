/* 2020. 09. 22
 * Creater : Gunhee Choi
 * Modifier : Hojin Shin
 * This file implements the functions of Simulation Class
*/

#include <time.h>
#include "zns_simulation.h"
#include "zns_simulation_datastructure.h"

ZNS_Simulation::ZNS_Simulation(char * path, int _zone_count, list<Workload_Data * > * workload_list) {
        zns_info_list = (struct zns_share_info *) malloc(sizeof(struct zns_share_info));
        m2_zns_init(path, zns_info_list);

        //ZNS SSD status print
        //m2_zns_init_print(zns_info_list);
        
        //init Argument
        Zone_count = _zone_count;
        Segment_count = SEGMENT_COUNT_IN_ZONE;
        Block_count = BLOCK_COUNT_IN_SEGMENT;
        total_segment_count = Zone_count * Segment_count;
        total_block_count = total_segment_count * Block_count;
        
        //init bitmap
        init_block_bitmap();
        init_segment_bitmap();
        init_zone_bitmap();

        this->workload_list = workload_list;

        current_i_block_bitmap = 0;
        
}

//***************** init function *****************//

void ZNS_Simulation::init_block_bitmap() {
    random_device rd;
    std::mt19937 mersenne(rd());
    std::uniform_int_distribution<> dis(0, 8192);
    int cls;
    int count[3] = { 0 };
    
    Block_bitmap = new SIM_Block[total_block_count];
    // Set all block data with random number generator
    cout<<endl<<"Set Block Data ...ing"<<endl;
    srand(time(0));
    for(int i=0; i<total_block_count; i++) {
        //cls = dis(mersenne);
        cls = rand();
        Block_bitmap[i].set_block_info(i);
        
        if (cls % 3 == 0) {
            Block_bitmap[i].set_data(0);
            count[0]++;
        } else if (cls % 3 == 1) {
            Block_bitmap[i].set_data(1);
            count[1]++;
        } else {
            Block_bitmap[i].set_data(2);
            count[2]++;
        }
    }
    for (int i = 0; i <= 2; i++) {
        cout << count[i] << endl;
    }
    
    cout<<"Finish Set Block Data ...ing"<<endl;

    cout<<endl<<"Block Info First & Last"<<endl;
    cout<< "#############################" <<endl;
    cout<< "[First Block Info]" << endl;
    print_block_info(0);
    cout<< "[Last Block Info]" << endl;
    print_block_info(total_block_count-1);
    cout<<endl;
}

void ZNS_Simulation::init_segment_bitmap() {
        int segment_state;
        int segment_state_cnt = 0;
        
        Segment_bitmap = new SIM_Segment[total_segment_count];
        
        for(int i=0; i<total_segment_count; i++) {
            for(int j = Segment_bitmap[i].get_i_start_block(); j < Segment_bitmap[i].get_i_start_block() + BLOCK_COUNT_IN_SEGMENT; j++) {
                if (Block_bitmap[j].get_data() == VALID_DATA) segment_state_cnt++;
            }
            if (segment_state_cnt > 307) segment_state = H1_STATUS_SEGMENT;
            else segment_state = C1_STATUS_SEGMENT;
            Segment_bitmap[i].set_segment_info(i, i * BLOCK_COUNT_IN_SEGMENT, segment_state);
            //cout << "segment count = " << segment_state_cnt << endl;
            segment_state_cnt = 0;
        }
        cout<<"Segment Info First & Last"<<endl;
        cout<< "#############################" <<endl;
        cout<< "[First Segment Info]" << endl;
        print_segment_info(0);
        cout<< "[Last Segment Info]" << endl;
        print_segment_info(total_segment_count-1);
        cout<<endl;

}

void ZNS_Simulation::init_zone_bitmap() {
        Zone_bitmap = new SIM_Zone[Zone_count];
        for(int i=0; i<Zone_count; i++) {
            Zone_bitmap[i].set_zone_info(i, i * SEGMENT_COUNT_IN_ZONE, i * SEGMENT_COUNT_IN_ZONE * SEGMENT_COUNT_IN_ZONE);
        }
        cout<<"Zone Info First & Last"<<endl;
        cout<< "#############################" <<endl;
        cout<< "[First Zone Info]" << endl;
        print_zone_info(0);
        cout<< "[Last Zone Info]" << endl;
        print_zone_info(Zone_count-1);
        cout<<endl;
}

//***************** End init function *****************//

int ZNS_Simulation::write_data() {

    /*
    void * buffer_128KB_write = malloc(512 * 8 * 32);
    memset(buffer_128KB_write, 99, 512 * 8 * 32);

    zns_zone_reset_request(zns_info_list->fd, 0);
    zns_write(zns_info_list, buffer_128KB_write, 512 * 8 * 32, 0, 0);

    return 0;
    */
}

int ZNS_Simulation::read_data() {
    cout<< "read data in func" <<endl;
    return 0;
}

int ZNS_Simulation::basic_zgc() {
    
    cout<< "Start Basic ZGC" <<endl;
    
    if(workload_list->size() == 0) {
        cout<< "Workload empty" <<endl;
        return -1;
    }

    list<Workload_Data * >::iterator iter;
    for (iter = workload_list->begin(); iter != workload_list->end(); ++iter){
        cout<< (*iter)->get_SN() << ", " << (*iter)->get_start_lba() <<endl;
        //(*iter)->print_workload_data();
    }




    return 0;
}

int ZNS_Simulation::lsm_zgc() {
    cout<< "Start LSM ZGC" <<endl;
    
    if(workload_list->size() == 0) {
        cout<< "Workload empty" <<endl;
        return -1;
    }

    list<Workload_Data * >::iterator iter;
    for (iter = workload_list->begin(); iter != workload_list->end(); ++iter){
        cout<< (*iter)->get_SN() << ", " << (*iter)->get_start_lba() <<endl;
        //(*iter)->print_workload_data();
    }

    return 0;
}

int ZNS_Simulation::request_write(int start_lba, int blocks) {
    int numofrequest = blocks/8;  
    int reqeust_lbs = start_lba;

    if( (blocks%8) != 0 ) {
        numofrequest += 1;
    }

    for(int i= 0; i<numofrequest; i++) {
        if(this->serialize_map.find(reqeust_lbs) == this->serialize_map.end()) {
            //Not found start lba
            this->serialize_map.insert(make_pair(reqeust_lbs, current_i_block_bitmap));
            current_i_block_bitmap++;
        } else {
            //Found start lba
            this->serialize_map.find(100)->second = current_i_block_bitmap;
        }
    }

    for(int i= 0; i<numofrequest; i++) {
        cout<< "count : " << i << "/" << (numofrequest-1) <<endl;
        reqeust_lbs += 8;
        cout<< "start_lba : " << reqeust_lbs <<endl;
        cout<<endl;
    }
    
    return 0;
}

int ZNS_Simulation::init_zones_write(int numofzones) {
    int i_zone, i_segment, i_block, i_bitmap;
    int offset;
    int write_result;

    void * dummy_data = new char[ZNS_BLOCK_SIZE];
    memset(dummy_data, 66, ZNS_BLOCK_SIZE);

    for(i_zone=0; i_zone<numofzones; i_zone++) {
        /*
        cout<< "Fill Zone " << i_zone <<endl;
        for(i_segment=0; i_segment<ZONE_IN_SEGMENT_COUNT; i_segment++) {

            for(i_block=0; i_block<SEGMENT_IN_BLOCK_COUNT; i_block++) {
                offset = get_offset_in_zone(i_segment, i_block);
                i_bitmap = get_i_bitmap(i_zone, i_segment, i_block);

                write_result = m2_zns_write(zns_info_list, dummy_data, ZNS_BLOCK_SIZE, i_zone, offset);
                //if(write_result == 0)
                    //block_bitmap[i_bitmap] = 1;
            }//end of blocks

        }//end of segments
        m2_zns_zone_finish(zns_info_list, i_zone);
        */
    }//end of zones

    free(dummy_data);
    return 0;
}

void ZNS_Simulation::init_zone_reset(int numofzones) {
    int i_zone;

    cout<< "Zone Init Reset" <<endl;
    for(i_zone=0; i_zone<numofzones; i_zone++) {
        m2_zns_zone_reset(zns_info_list, i_zone);
    }
}

int ZNS_Simulation::get_offset_in_zone(int i_segment, int i_block) {
    return 0; //i_segment * ZONE_IN_SEGMENT_COUNT * SIM_BLOCK_IO_DEFAULT + i_block * SIM_BLOCK_IO_DEFAULT;
}

int ZNS_Simulation::get_i_bitmap(int i_zone, int i_segment, int i_block) {
    return 0; //i_zone * ZONE_IN_BLOCK_COUNT + i_segment * ZONE_IN_SEGMENT_COUNT + i_block;
}

//***************** test workload function *****************//

int ZNS_Simulation::setting_random_bitmap() {
    int i_block, i_segment, i_zone;

    random_device rd;
    std::mt19937 mersenne(rd());
    std::uniform_int_distribution<> dis(0, total_block_count);

    /*
    i_block =
        i_zone * SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT
        + i_segment * BLOCK_COUNT_IN_SEGMENT
        + offset
    */
    i_block =  dis(mersenne);

    //find zone
    i_zone = i_block / (SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT);

    //find segment
    i_segment = i_block - (i_zone * SEGMENT_COUNT_IN_ZONE * BLOCK_COUNT_IN_SEGMENT);
    i_segment = i_segment / BLOCK_COUNT_IN_SEGMENT;


    
    return 0;
}

//***************** End test workload function *****************//

//***************** print function *****************//

void ZNS_Simulation::print_block_info(int offset) {
    Block_bitmap[offset].print_block_info();
}

void ZNS_Simulation::print_segment_info(int offset) {
    Segment_bitmap[offset].print_segment_info();
}

void ZNS_Simulation::print_zone_info(int offset) {
    Zone_bitmap[offset].print_zone_info();
}

void ZNS_Simulation::print_segment_block_bitmap(int i_segment) {
    int start_i_block = Segment_bitmap[i_segment].get_i_start_block();
    int end_i_block = start_i_block + BLOCK_COUNT_IN_SEGMENT;
    int list_cnt = 0;

    cout<< "Segment " << i_segment << " block_bitmap" <<endl;
    for(int i_block=start_i_block; i_block<end_i_block; i_block++) {
        list_cnt++;
        cout << Block_bitmap[i_block].get_data() << " ";
        if((list_cnt % 20) == 0)
            cout<<endl;
    }
    cout<<endl;
}

void ZNS_Simulation::print_zone_block_bitmap(int i_zone) {
    int start_i_block = Zone_bitmap[i_zone].get_i_start_block();
    int end_i_block = start_i_block + BLOCK_COUNT_IN_SEGMENT * SEGMENT_COUNT_IN_ZONE;
    int list_cnt = 0;

    cout<< "Zone " << i_zone << " block_bitmap" <<endl;
    for(int i_block=start_i_block; i_block<end_i_block; i_block++) {
        list_cnt++;
        cout << Block_bitmap[i_block].get_data() << " ";
        if((list_cnt % 512) == 0)
            cout<<endl;
    }
    cout<<endl;
}

void ZNS_Simulation::print_zone_segment_bitmap(int i_zone) {
    int start_i_segment = Zone_bitmap[i_zone].get_i_start_segment();
    int end_i_segment = start_i_segment + SEGMENT_COUNT_IN_ZONE;
    int t_status;
    int list_cnt = 0;

    cout<< "Zone " << i_zone << " segment_bitmap" <<endl;
    for(int i_segment=start_i_segment; i_segment<end_i_segment; i_segment++) {
        list_cnt++;
        t_status = Segment_bitmap[i_segment].get_status();

        if( t_status == DEFAULT_STATUS_SEGMENT )
            cout<< "D0 ";
        else if( t_status == H1_STATUS_SEGMENT )
            cout<< "H1 ";
        else if( t_status == C1_STATUS_SEGMENT )
            cout<< "C1 ";
        else if( t_status == C2_STATUS_SEGMENT )
            cout<< "C2 ";

        if((list_cnt % 20) == 0)
            cout<<endl;
    }
    cout<<endl;
}


//***************** End print function *****************//