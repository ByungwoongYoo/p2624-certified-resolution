// Independent checker for the P2624 exhaustive lift-tree certificate.
//
// This checker deliberately does NOT reproduce the generator's MRV rule.
// At a branch byte it accepts any named unassigned residue class, independently
// computes every currently feasible lift mask for that class, and requires one
// recursively valid child proof for every such mask. At a dead byte it verifies
// that the named class has no feasible lift mask. Thus the certificate, not the
// search program, supplies the exhaustive tree.

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
using Counts=std::array<unsigned char,Q>;using Sig=std::array<unsigned char,51>;
int dist(int a,int b){int z=std::abs(a-b);return std::min(z,N-z);}int limit(int d){return d==50?1:2;}
std::array<std::vector<unsigned>,4> domains0(){std::array<std::vector<unsigned>,4>d;d[0]={0};for(unsigned m=1;m<32;++m){int w=__builtin_popcount(m);if(w<=3)d[w].push_back(m);}return d;}const auto DOM=domains0();
std::vector<int> pts(int r,unsigned m){std::vector<int>p;for(int q=0;q<5;++q)if(m&(1u<<q))p.push_back(r+20*q);return p;}
Sig own(int r,unsigned m){Sig s{};auto p=pts(r,m);for(size_t i=0;i<p.size();++i)for(size_t j=i+1;j<p.size();++j)++s[dist(p[i],p[j])];return s;}
Sig between(int r,unsigned a,int s,unsigned b){Sig z{};auto x=pts(r,a),y=pts(s,b);for(int u:x)for(int v:y)++z[dist(u,v)];return z;}
Counts parse(const std::string&line){if(line.size()!=20)throw std::runtime_error("catalog line length");Counts c{};int sum=0;for(int i=0;i<20;++i){if(line[i]<'0'||line[i]>'3')throw std::runtime_error("catalog digit");c[i]=line[i]-'0';sum+=c[i];}if(sum!=14)throw std::runtime_error("catalog sum");return c;}
uint32_t read_u32(std::istream&i){uint32_t x=0;for(int b=0;b<4;++b){int c=i.get();if(c==EOF)throw std::runtime_error("truncated u32");x|=(uint32_t)(unsigned char)c<<(8*b);}return x;}
uint64_t read_u64(std::istream&i){uint64_t x=0;for(int b=0;b<8;++b){int c=i.get();if(c==EOF)throw std::runtime_error("truncated u64");x|=(uint64_t)(unsigned char)c<<(8*b);}return x;}

struct CheckState{
    std::vector<int> residues;std::vector<std::vector<unsigned>> dom;std::vector<std::vector<Sig>> internal;std::vector<std::vector<std::vector<Sig>>> cross;std::vector<int> assigned;std::array<unsigned char,51> used{};int assigned_count=0;std::istream* proof=nullptr;uint64_t consumed=0,expected=0;
    void setup(const Counts&occ){for(int r=0;r<20;++r)if(occ[r])residues.push_back(r);int n=residues.size();dom.resize(n);internal.resize(n);assigned.assign(n,-1);cross.resize(n,std::vector<std::vector<Sig>>(n));for(int i=0;i<n;++i){dom[i]=DOM[occ[residues[i]]];for(unsigned m:dom[i])internal[i].push_back(own(residues[i],m));}for(int i=0;i<n;++i)for(int j=i+1;j<n;++j){auto&t=cross[i][j];for(unsigned a:dom[i])for(unsigned b:dom[j])t.push_back(between(residues[i],a,residues[j],b));}}
    const Sig& pair(int i,int di,int j,int dj)const{return i<j?cross[i][j][di*dom[j].size()+dj]:cross[j][i][dj*dom[i].size()+di];}
    bool candidate(int v,int di,Sig&delta)const{delta=internal[v][di];for(int j=0;j<(int)residues.size();++j){int dj=assigned[j];if(dj<0)continue;const Sig&s=pair(v,di,j,dj);for(int d=1;d<=50;++d)delta[d]=(unsigned char)(delta[d]+s[d]);}for(int d=1;d<=50;++d)if(used[d]+delta[d]>limit(d))return false;return true;}
    void add(const Sig&s,int sign){for(int d=1;d<=50;++d){if(sign>0)used[d]=(unsigned char)(used[d]+s[d]);else used[d]=(unsigned char)(used[d]-s[d]);}}
    uint8_t byte(){if(consumed>=expected)throw std::runtime_error("tree exceeds declared node count");int c=proof->get();if(c==EOF)throw std::runtime_error("truncated proof tree");++consumed;return (uint8_t)c;}
    void verify_node(){
        if(assigned_count==(int)residues.size())throw std::runtime_error("certificate reaches a complete valid lift");
        uint8_t code=byte();bool dead=(code&0x80)!=0;if(code&0x70)throw std::runtime_error("reserved proof bits set");int v=code&0x0f;if(v<0||v>=(int)residues.size()||assigned[v]>=0)throw std::runtime_error("invalid proof branch variable");
        std::vector<std::pair<int,Sig>> valid;for(int di=0;di<(int)dom[v].size();++di){Sig d{};if(candidate(v,di,d))valid.push_back({di,d});}
        if(dead){if(!valid.empty())throw std::runtime_error("dead node has a feasible lift mask");return;}
        if(valid.empty())throw std::runtime_error("branch node has no feasible lift mask; should be dead");
        for(auto&item:valid){assigned[v]=item.first;++assigned_count;add(item.second,+1);verify_node();add(item.second,-1);--assigned_count;assigned[v]=-1;}
    }
};
}

int main(int argc,char**argv){
    try{
        if(argc!=3){std::cerr<<"usage: check_lift_tree catalog.txt certificate.tree\n";return 2;}
        std::ifstream cat(argv[1]);if(!cat)throw std::runtime_error("cannot open catalog");std::vector<Counts>rows;std::string line;while(std::getline(cat,line))if(!line.empty())rows.push_back(parse(line));
        std::ifstream pr(argv[2],std::ios::binary);if(!pr)throw std::runtime_error("cannot open certificate");char magic[8];pr.read(magic,8);const std::string got(magic,pr.gcount());if(got!="P2624T1\n")throw std::runtime_error("bad certificate magic");uint32_t nc=read_u32(pr);if(nc!=rows.size())throw std::runtime_error("certificate/catalog config count mismatch");
        auto start=std::chrono::steady_clock::now();uint64_t total=0;
        for(size_t idx=0;idx<rows.size();++idx){uint64_t expect=read_u64(pr);CheckState s;s.setup(rows[idx]);s.proof=&pr;s.expected=expect;s.verify_node();if(s.consumed!=expect)throw std::runtime_error("declared subtree node count mismatch at config "+std::to_string(idx));total+=expect;if((idx+1)%10==0||idx+1==rows.size()){double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cerr<<"c verified config="<<(idx+1)<<"/"<<rows.size()<<" nodes="<<total<<" sec="<<sec<<"\n";}}
        if(pr.peek()!=EOF)throw std::runtime_error("trailing bytes after final proof tree");double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();std::cout<<"s VERIFIED exhaustive lift-tree certificate configs="<<rows.size()<<" nodes="<<total<<" seconds="<<sec<<"\n";return 0;
    }catch(const std::exception&e){std::cerr<<"s NOT VERIFIED: "<<e.what()<<"\n";return 1;}
}
