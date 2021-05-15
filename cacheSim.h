
#include <vector>
#include <time.h>
#include <bitset>
#include <math.h>

#define WRITE_ALLOCATE 1
#define NO_WRITE_ALLOCATE 0

void DecodeAddrs(int addrs , int& set , int& tag, int SetSize ){

    std::bitset<32> ADDRS(addrs);
    set  = 0;
    for(int i=5 ; i<5+SetSize ;i++) set+= ADDRS[i]*pow(2,i);
    tag = 0;
    for(int i = 5+SetSize ; i< 32 ;i++ ) tag += ADDRS[i]*pow(2,i);



}

class Block{

    public:
        Block():
            dirty(0),valid(0){
                time(&timer);

            }
        void validate(){
            valid = 1;
        }
        void Invalidate(){
            valid = 0;
        }
        void set_accessed(){
            time(&timer);
        }
        void set_dirty(){
            dirty = 1;
        }


    bool dirty;
    bool valid;
    time_t timer;

};

class line{
    public:
        line(int tag=0):
            Tag(tag){}
        
        bool IsValid(){ return block.valid ;}
        bool IsDirty(){return block.dirty;}
        void validate(){
            block.validate();
            block.set_accessed();
        }
        void read_line(){
            block.set_accessed();
        }
        void write_line(){
            block.set_dirty();
            block.set_accessed();
        }
        void pop_line(){
            block.Invalidate();
        }
        void update_line(){block.set_accessed();}
        time_t& get_time(){return block.timer;}
        int Tag;
        Block block;
};

class Way{

    public:
        Way(int SIZE = 1 ):
            Waysize(SIZE){
                lines.resize(SIZE);
            }
        
        void push(int addrs);
        bool try_push(int addrs);
        bool look(int addrs);
        bool look(int set,int tag) { return lines[set].Tag == tag;}
        void read(int addrs);
        void write(int addrs);
        time_t& get_time(int addrs);
        line& get_line(int set);
        bool IsDirty(int set){ return lines[set].IsDirty();}
        void update(int set){ lines[set].update_line();}


        std::vector<line> lines;
        int Waysize;
        friend void DecodeAddrs(int addrs , int& set , int& tag , int SetSize);


};

class Victim{
public :
    int tag;
    int set;
    bool dirty;
};

class Cache{

    public:
        Cache(int memTime=0, int CacheSize_=0 , int BlockSize_ =0, int ways_num_=0 , int AccessTime_=0 , int MissPolicy_=0):
            MemTime(memTime),CacheSize(CacheSize_),BlockSize(BlockSize_),ways_num(ways_num_+1),AccessTime(AccessTime_),
            MissPolicy(MissPolicy_),Misses(0),hits(0), AccCount(0)
            {
                Ways.resize(ways_num_);
                Waysize = CacheSize_/(BlockSize_*ways_num_) ;
                for(int i=0 ; i<ways_num_ ; i++)
                    Ways[i] = Way(Waysize);

            }
        
        bool ReadFrom(int addrs);
        bool WriteTo(int addrs);
        void Insert(int addrs, Victim &victim);
        void Invalidate(int set, int victimTag);
        void update(int set , int tag);
        friend class Stats;

        int Waysize;
        int MemTime;
        int CacheSize ; //cache size in bytes
        int BlockSize; //block size in bytes
        int ways_num; //assosotivity of cache
        int AccessTime;
        int MissPolicy;
        int Misses;
        int hits;
        int AccCount;

    std::vector<Way> Ways;



};

class CacheControl{

    public:
        CacheControl(int memTime, int L1Size, int L2Size, int BlockSize_, int L1ways_num_, int L2ways_num_, int L1AccessTime_, int L2AccessTime_, int MissPolicy_) :
        tAccess(0), MemTime(memTime)
        {
            L1 = Cache(memTime, L1Size ,BlockSize_ ,L1ways_num_ ,L1AccessTime_ ,MissPolicy_);
            L2 = Cache(memTime, L2Size ,BlockSize_ ,L2ways_num_ ,L2AccessTime_ ,MissPolicy_);
            MissPolicy = MissPolicy_;
        }
        void Read(int addrs);
        void Write(int addrs);

        double L1Stats() { return L1.Misses/L1.AccCount; }
        double L2Stats() { return L2.Misses/L2.AccCount; }
        double AvgAccTime() { return tAccess/tAccessCount; }
       // friend class Stats;

        int MemTime;
        Cache L1;
        Cache L2; //LLC
        int MissPolicy;
        int tAccess;
        int tAccessCount;
};

//class Stats{
//public:
//
//    double L1Stats() { return CacheControl::L1.Misses/CacheControl::L1.AccCount; }
//    double L2Stats() { return L2.Misses/L2.AccCount; }
//    double AvgAccTime() { return tAccess/tAccessCount; }
//
//};

/*******************************Way**********************************/
void Way::push(int addrs ){
    int  set , tag;
    DecodeAddrs(addrs , set ,tag, log2(Waysize) );
    lines[set].validate();
    lines[set].Tag = tag;

}
bool Way::try_push(int addrs){

    int  set , tag;
    DecodeAddrs(addrs , set ,tag, log2(Waysize) );
    if(!lines[set].IsValid()) 
        return true;
    
    return false;

}
bool Way::look(int addrs){
    int  set , tag;
    DecodeAddrs(addrs , set ,tag, log2(Waysize) );
    if(lines[set].IsValid() && lines[set].Tag == tag)
        return true;
    
    return false;
}

void Way::read(int addrs){
    int  set , tag;
    DecodeAddrs(addrs , set ,tag, log2(Waysize) );
    lines[set].read_line();
}
void Way::write(int addrs){
    int  set , tag;
    DecodeAddrs(addrs , set ,tag, log2(Waysize) );
    lines[set].write_line();
}

time_t& Way::get_time(int addrs){
    int  set , tag;
    DecodeAddrs(addrs , set ,tag, log2(Waysize) );
    
    return lines[set].get_time();
}
line& Way::get_line(int set){
    
    return lines[set];    
}

/***********************************Cache**************************************/
bool Cache::ReadFrom(int addrs){

    bool found = false;
    for(int i = 0 ; i<ways_num ; i++){
        if(Ways[i].look(addrs)){
            Ways[i].read(addrs);
            hits++;
            return true;
        }
    }
    Misses++;
    return false ; //miss

}
bool Cache::WriteTo(int addrs){

    bool found = false;
    for(int i = 0 ; i<ways_num ; i++){
        if(Ways[i].look(addrs)){
            Ways[i].write(addrs);
            hits++;
            return true;
        }
    }
    Misses++;
    return false ; //miss

}

void Cache::Invalidate(int set, int victimTag){
    for(int i = 0 ; i<ways_num ; i++) {
        if (Ways[i].look(set, victimTag)) {
            Ways[i].get_line(set).block.valid = 0;
        }
    }
}

void Cache::Insert(int addrs , Victim& victim){
    
    bool pushed = false;
    int  set , tag;
    DecodeAddrs(addrs , set ,tag, log2(Waysize) );

    for(int i = 0 ;i<ways_num;i++){
        if(Ways[i].try_push(addrs)){
            Ways[i].push(addrs);
            victim.tag = -1;
            victim.set = -1;
            victim.dirty = 0;
            pushed = true;
        }
    }

    if(pushed == false){
        double seconds , max = 0;
        int victim_idx;
        time_t now;
        time(&now);

        for(int i=0 ; i<ways_num ;i++){
            if(difftime(now,Ways[i].get_time(addrs))>max){
                max = difftime(now,Ways[i].get_time(addrs));
                victim_idx = i;

            }
        }
        victim.tag = Ways[victim_idx].get_line(set).Tag;
        victim.set = set;
        victim.dirty = Ways[victim_idx].IsDirty(set);
        Ways[victim_idx].push(addrs);
    }
}
void Cache::update(int set,int tag){ //mark

    for(int i=0 ;i<ways_num ;i++){
        if(Ways[i].look(set , tag)){
            Ways[i].update(set);
        }
    }
}
/********************************************CacheControl**********************************/

void CacheControl::Read(int addrs){
    
    bool state = false;
    Victim victim;

    tAccessCount++;
    L1.AccCount++;
    state = L1.ReadFrom(addrs);

    if (state) {
    tAccess += L1.AccessTime;
    }

    if(MissPolicy == WRITE_ALLOCATE){
        if(state == false){
            state = L2.ReadFrom(addrs);

            L2.AccCount++;
            if (state) {
              tAccess += L1.AccessTime + L2.AccessTime ;
             }

            if(state == false){
                tAccess += L1.AccessTime + L2.AccessTime + MemTime;
                L2.Insert(addrs , victim);
                if(victim.tag != -1 ){
                    L1.Invalidate(victim.set,victim.tag);
                }
            }

            L1.Insert(addrs , victim);
            if(victim.tag != -1 && victim.dirty) {
                L2.update(victim.set, victim.tag);
            }
            L1.ReadFrom(addrs);
        }
    }
}
void CacheControl::Write(int addrs){
    
    bool state = false;
    Victim victim;

    tAccessCount++;
    L1.AccCount++;

    state = L1.WriteTo(addrs);
    if (state) {
        tAccess += L1.AccessTime;
    }
    if(state == false){
        state = L2.WriteTo(addrs);

        L2.AccCount++;
        if (state) {
            tAccess += L1.AccessTime + L2.AccessTime ;
        }

        if(MissPolicy == WRITE_ALLOCATE) {
            if (state == false) {

                tAccess += L1.AccessTime + L2.AccessTime + MemTime;
                L2.Insert(addrs, victim);
                if (victim.tag != -1) {
                    L1.Invalidate(victim.set, victim.tag);
                }
            }
                L1.Insert(addrs, victim);
                if (victim.tag != -1 && victim.dirty) {
                    L2.update(victim.set, victim.tag);
                }
                L1.WriteTo(addrs);

        }
    
    }
}

