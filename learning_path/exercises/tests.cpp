#include "labs.hpp"
#include "pusch_lab.hpp"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
using namespace learner;
static void require(bool b) {if(!b) throw std::runtime_error("check failed");}
static void near(double a,double b) {require(std::abs(a-b)<1e-10);}
template<class F> static void rejects(F f) {bool bad=false;try{f();}catch(const std::invalid_argument&){bad=true;}require(bad);}
int main(int argc,char** argv) {
 const std::map<std::string,std::function<void()>> cases={
 {"L01",[]{auto v=data_re();require(v.size()==1380);require(v.front()==60 && v.back()==6779);
   require(std::set<unsigned>(v.begin(),v.end()).size()==v.size());
   require(std::find(v.begin(),v.end(),1260)==v.end());require(std::find(v.begin(),v.end(),1261)!=v.end());}},
 {"L02",[]{for(auto seed:{0U,1U,42U,0x7fffffffU}) {
    // Independent packed-register recurrence, not the solution's array recurrence.
    auto v=learner::gold(seed,257);require(v.size()==257);std::uint32_t a=1,b=seed;
    for(unsigned i=0;i<1857;++i) {if(i>=1600)require(v[i-1600]==((a^b)&1));
      auto fa=((a>>3)^a)&1;auto fb=((b>>3)^(b>>2)^(b>>1)^b)&1;
      a=(a>>1)|(fa<<30);b=(b>>1)|(fb<<30);}
   }}},
 {"L03",[]{auto z=qam16({0,0,0,0});near(z.real(),1/std::sqrt(10.));near(z.imag(),1/std::sqrt(10.));
   z=qam16({1,0,1,0});near(z.real(),-3/std::sqrt(10.));near(z.imag(),1/std::sqrt(10.));
   double e=0;for(unsigned n=0;n<16;++n){Bits b;for(int k=3;k>=0;--k)b.push_back((n>>k)&1);e+=std::norm(qam16(b));}near(e/16,1);
   rejects([]{qam16({0,1});});}},
 {"L04",[]{require(dmrs_init(2,2,42)==345374804U);require(dmrs_init(0,0,0)==131072U);
    rejects([]{dmrs_init(0,14,0);});}},
 {"L05",[]{std::vector<IQ>x={{1,0},{0,1},{-1,0},{0,-1}},y;IQ h(.8,.3);for(auto z:x)y.push_back(h*z);
   auto a=flat_ls(x,y);near(a.real(),.8);near(a.imag(),.3);rejects([]{flat_ls({},{});});}},
 {"L06",[]{auto b=pusch::bytes_to_bits({'1','2','3','4','5','6','7','8','9'});require(learner::crc16(b)==0x31c3);
   require(learner::crc16({})==0);auto c=pusch::attach_crc(b,pusch::Crc::CRC16);require(learner::crc16(c)==0);
   c[4]^=1;require(learner::crc16(c)!=0);}},
 {"L07",[]{auto r=check_node({2,-3,4},.75);require(r.size()==3);near(r[0],-2.25);near(r[1],1.5);near(r[2],-1.5);
    r=check_node({2,2,-4},1);near(r[0],-2);near(r[1],-2);near(r[2],2);
    r=check_node({0,2,-3},1);near(r[0],-2);near(r[1],0);near(r[2],0);}},
 {"L08",[]{require(rm_positions(0,8)==std::vector<unsigned>({0,4,1,5,2,6,3,7}));
    require(rm_positions(1,8)==std::vector<unsigned>({26,30,27,31,28,32,29,33}));
    require(rm_positions(3,202).size()==202);rejects([]{rm_positions(4,8);});}},
 {"L09",[]{auto r=combine({0,3,0},{1,-2,3},5);require(r==std::vector<double>({4,0,0,-2,0}));
    rejects([]{combine({5},{1},5);});}},
 {"L10",[]{auto r=qpsk_llr({1/std::sqrt(2.),-1/std::sqrt(2.)},.5);require(r.size()==2);near(r[0],4);near(r[1],-4);
    rejects([]{qpsk_llr({1,0},0);});}}
 };
 if(argc!=2 || (!cases.count(argv[1]) && std::string(argv[1])!="all")) {std::cerr<<"usage: nr_labs L01..L10|all\n";return 2;}
 int failed=0;
 for(const auto& [name,f]:cases) if(std::string(argv[1])=="all" || name==argv[1]) {
   try{f();std::cout<<"PASS "<<name<<'\n';}catch(const std::exception& e){++failed;std::cout<<"FAIL "<<name<<": "<<e.what()<<'\n';}
 }
 return failed?1:0;
}
