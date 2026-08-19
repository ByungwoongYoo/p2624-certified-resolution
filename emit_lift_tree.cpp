// P2624 exhaustive lift-tree certificate generator.
//
// For each canonical modulo-20 occupancy vector, this program searches every
// choice of lifts in Z_100. It writes a static proof tree. Each proof byte says
// either "branch on this residue class" or "this residue class has no feasible
// lift mask". The independent checker does not trust the MRV heuristic: it
// verifies the class named by each byte and requires a proof child for every
// feasible lift mask.

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
constexpr int N=100,Q=20,F=5,K=14;
using Counts=std::array<unsigned char,Q>;
using Sig=std::array<unsigned char,51>;

int cd(int a,int b){int d=std::abs(a-b);return std::min(d,N-d);} 
int cap(int d){return d==50?1:2;}

std::array<std::vector<unsigned>,4> make_domains(){
    std::array<std::vector<unsigned>,4> z;
    z[0].push_back(0);
    for(unsigned m=1;m<(1u<<F);++m){int w=__builtin_popcount(m);if(w<=3)z[w].push_back(m);}return z;
}
const auto MASKS=make_domains();

std::vector<int> points(int r,unsigned m){std::vector<int>v;for(int q=0;q<F;++q)if((m>>q)&1u)v.push_back(r+Q*q);return v;}
Sig internal_sig(int r,unsigned m){Sig s{};auto p=points(r,m);for(size_t i=0;i<p.size();++i)for(size_t j=i+1;j<p.size();++j)++s[cd(p[i],p[j])];return s;}
Sig cross_sig0(int r,unsigned a,int s,unsigned b){Sig z{};auto x=points(r,a),y=points(s,b);for(int u:x)for(int v:y)++z[cd(u,v)];return z;}

struct Solver {
    Counts occ{};
    std::vector<int> residues;
    std::vector<std::vector<unsigned>> domains;
    std::vector<std::vector<Sig>> internal;
    std::vector<std::vector<std::vector<Sig>>> cross;
    std::vector<int> assigned;
    std::array<unsigned char,51> used{};
    int assigned_count=0;
    uint64_t nodes=0;
    std::ostream* proof=nullptr;

    void prepare(){
        for(int r=0;r<Q;++r)if(occ[r])residues.push_back(r);
        int c=residues.size();domains.resize(c);internal.resize(c);assigned.assign(c,-1);cross.resize(c,std::vector<std::vector<Sig>>(c));
        for(int i=0;i<c;++i){domains[i]=MASKS[occ[residues[i]]];for(unsigned m:domains[i])internal[i].push_back(internal_sig(residues[i],m));}
        for(int i=0;i<c;++i)for(int j=i+1;j<c;++j){auto&t=cross[i][j];t.reserve(domains[i].size()*domains[j].size());for(unsigned a:domains[i])for(unsigned b:domains[j])t.push_back(cross_sig0(residues[i],a,residues[j],b));}
    }
    const Sig& cs(int i,int di,int j,int dj)const{return i<j?cross[i][j][di*domains[j].size()+dj]:cross[j][i][dj*domains[i].size()+di];}
    bool delta_for(int v,int di,Sig& d)const{
        d=internal[v][di];
        for(int o=0;o<(int)residues.size();++o){int od=assigned[o];if(od<0)continue;const Sig&p=cs(v,di,o,od);for(int x=1;x<=50;++x)d[x]=(unsigned char)(d[x]+p[x]);}
        for(int x=1;x<=50;++x)if(used[x]+d[x]>cap(x))return false;return true;
    }
    void apply(const Sig&d,int sign){for(int x=1;x<=50;++x){if(sign>0)used[x]=(unsigned char)(used[x]+d[x]);else used[x]=(unsigned char)(used[x]-d[x]);}}
    void put(uint8_t b){proof->put((char)b);if(!*proof)throw std::runtime_error("certificate write failure");++nodes;}

    bool dfs(){
        if(assigned_count==(int)residues.size())return true; // a full valid lift: cannot certify UNSAT
        int best=-1;std::vector<std::pair<int,Sig>> bestvals;
        for(int v=0;v<(int)residues.size();++v){
            if(assigned[v]>=0)continue;
            std::vector<std::pair<int,Sig>> valid;valid.reserve(domains[v].size());
            for(int di=0;di<(int)domains[v].size();++di){Sig d{};if(delta_for(v,di,d))valid.push_back({di,d});}
            if(valid.empty()){
                put((uint8_t)(0x80u|v));
                return false;
            }
            if(best<0||valid.size()<bestvals.size()){best=v;bestvals=std::move(valid);}
        }
        if(best<0)throw std::runtime_error("no branch variable");
        put((uint8_t)best);
        for(const auto&item:bestvals){int di=item.first;const Sig&d=item.second;assigned[best]=di;++assigned_count;apply(d,+1);if(dfs())return true;apply(d,-1);--assigned_count;assigned[best]=-1;}
        return false;
    }
};

Counts parse(const std::string&line){if(line.size()!=Q)throw std::runtime_error("bad catalog line length");Counts c{};int sum=0;for(int i=0;i<Q;++i){if(line[i]<'0'||line[i]>'3')throw std::runtime_error("bad occupancy digit");c[i]=line[i]-'0';sum+=c[i];}if(sum!=K)throw std::runtime_error("occupancy does not sum to 14");return c;}
void write_u32(std::ostream&o,uint32_t x){for(int i=0;i<4;++i)o.put((char)((x>>(8*i))&255));}
void write_u64(std::ostream&o,uint64_t x){for(int i=0;i<8;++i)o.put((char)((x>>(8*i))&255));}
}

int main(int argc,char**argv){
    try{
        if(argc!=3){std::cerr<<"usage: emit_lift_tree catalog.txt output.tree\n";return 2;}
        std::ifstream in(argv[1]);if(!in)throw std::runtime_error("cannot open catalog");std::vector<Counts> rows;std::string line;while(std::getline(in,line))if(!line.empty())rows.push_back(parse(line));if(rows.empty())throw std::runtime_error("empty catalog");
        std::fstream out(argv[2],std::ios::binary|std::ios::out|std::ios::trunc);if(!out)throw std::runtime_error("cannot open output");
        const char magic[8]={'P','2','6','2','4','T','1','\n'};out.write(magic,8);write_u32(out,(uint32_t)rows.size());
        auto allstart=std::chrono::steady_clock::now();uint64_t total=0;
        for(size_t idx=0;idx<rows.size();++idx){
            std::streampos countpos=out.tellp();write_u64(out,0);std::streampos datapos=out.tellp();
            Solver s;s.occ=rows[idx];s.proof=&out;s.prepare();bool sat=s.dfs();if(sat)throw std::runtime_error("SAT lift found at config "+std::to_string(idx));
            std::streampos endpos=out.tellp();out.seekp(countpos);write_u64(out,s.nodes);out.seekp(endpos);total+=s.nodes;
            if((idx+1)%10==0||idx+1==rows.size()){double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-allstart).count();std::cerr<<"c config="<<(idx+1)<<"/"<<rows.size()<<" nodes_total="<<total<<" bytes="<<(long long)out.tellp()<<" sec="<<sec<<"\n";}
        }
        out.flush();double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-allstart).count();std::cout<<"s CERTIFICATE GENERATED configs="<<rows.size()<<" nodes="<<total<<" seconds="<<sec<<"\n";return 0;
    }catch(const std::exception&e){std::cerr<<"ERROR: "<<e.what()<<"\n";return 2;}
}
