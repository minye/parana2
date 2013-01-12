#include "TreeUtils.hpp"
#include "MultiOpt.hpp"
#include <boost/heap/fibonacci_heap.hpp>
#include <boost/heap/pairing_heap.hpp>
#include <Bpp/Phyl/Io/Nhx.h>
using boost::heap::fibonacci_heap;
using boost::heap::pairing_heap;

namespace Utils {

    double round3( double num ) {
        double result = num * 1000;
        result = std::floor(result);
        result = result / 1000;
        return result;
    };

    bool advanceElems( vector<size_t>& ptrs, const vector<size_t>& sizes ) {

        assert( ptrs.size() == sizes.size() );
        // Find the last element
        size_t ctr = sizes.size()-1;
        for ( auto rp = ptrs.rbegin(); rp != ptrs.rend(); ++rp ) {
            if ( *rp < sizes[ctr]-1 ) {
                ++(*rp);
                return true;
            } else {
                if ( rp+1 == ptrs.rend() ) {
                    return false;
                } else {
                    (*rp) = 0;
                }
            }
            --ctr;
        }

    }

    template <typename pqT, typename pqCompT>
    bool appendNext( double score,
                     const vector<size_t>& inds,
                     const vector<size_t>& sizes,
                     vector<pqT>& pq,
                     pqCompT& pqComp,
                     std::function< double(const vector<size_t>&) >& computeScore ) {

        size_t i = 0;
        while ( i < inds.size() ) {
            vector<size_t> newInds(inds);
            newInds[i]++;
            if ( newInds[i] < sizes[i] ) {
                pq.push_back( make_tuple( computeScore(newInds), newInds ) );
                std::push_heap( pq.begin(), pq.end(), pqComp );
            }
            if (inds[i] != 0) { return true; }
            i += 1;
        }

        return true;
    }

    template <typename pqT>//, typename pqCompT>
    bool appendNextWithEdge( const size_t& eid,
                             const vector<size_t>& inds,
                             const vector<size_t>& sizes,
                             pqT& pq,
                             //vector<pqT>& pq,
                             //pqCompT& pqComp,
                             std::function< double(const size_t& eid, const vector<size_t>&) >& computeScore ) {

        size_t i = 0;
        while ( i < inds.size() ) {
            vector<size_t> newInds(inds);
            newInds[i]++;
            if ( newInds[i] < sizes[i] ) {
                pq.push( make_tuple( computeScore(eid, newInds), eid, newInds ) );
            }
            if (inds[i] != 0) { return true; }
            i += 1;
        }

        return true;
    }

    /*
    template <typename pqT>//, typename pqCompT>
    bool appendNextWithEdgeOrig( const size_t& eid,
                                 const vector<size_t>& inds,
                                 const vector<size_t>& sizes,
                                 pqT& pq,
                                 std::function< double(const size_t& eid, const vector<size_t>&) >& computeScore ) {

        size_t i = 0;
        while ( i < inds.size() ) {
            vector<size_t> newInds(inds);
            newInds[i]++;
            if ( newInds[i] < sizes[i] ) {
                double score = computeScore(eid, newInds);
                bool exists = false;
                auto it = pq.ordered_begin();
                while( it != pq.ordered_end() && !exists ) {
                    double dscore = get<0>(*it);
                    size_t oeid = get<1>(*it);
                    if ( dscore == score && eid == oeid ) {
                        auto oinds = get<2>(*it);
                        size_t sct = 0;
                        for( size_t j = 0; j < oinds.size(); ++j ) {
                            if( newInds[j] != oinds[j] ){ break; } else { sct += 1; }
                        }
                        if ( sct == oinds.size() ) { exists = true; }
                    }
                    if ( dscore > score ) { break; }
                    ++it;
                }

                if (!exists) {
                    pq.push( make_tuple( computeScore(eid, newInds), eid, newInds ) );
                }
            }
            //if (inds[i] != 0) { return true; }
            i += 1;
        }

        return true;
    }
*/

    typedef tuple<double, vector<size_t> > dvsT;
    class QueueCmp {
    public:
        bool cmpArrays( const vector<size_t>& a, const vector<size_t>& b) {
            assert(a.size() == b.size());
            size_t i = 0;
            while (i < a.size() && a[i] == b[i] ) {
                i += 1;
            }
            if (i == a.size()) {
                return true;
            } else {
                return a[i] > b[i];
            }
        }

        bool operator() ( const dvsT& lhs, const dvsT& rhs ) {
                return get<0>(lhs) >= get<0>(rhs);
        }
    };



    namespace Trees {
        typedef shared_ptr<bpp::TreeTemplate<bpp::Node>> TreePtrT;


        bool differentExtantNetworks( const TreeInfo& tinfo, int u, int v ) {
            //auto specU = dynamic_cast< bpp::BppString* >(tinfo.tree->getNodeProperty(u, "S"))->toSTL();
            //auto specV = dynamic_cast< bpp::BppString* >(tinfo.tree->getNodeProperty(v, "S"))->toSTL();
            //return specU != specV;
            if ( u == v ) { return false; }
            auto& enetsU = tinfo.enets.find(u)->second;
            auto& enetsV = tinfo.enets.find(v)->second;
            
            for (auto uit = enetsU.cbegin(); uit != enetsU.cend(); ++uit) {
                if ( enetsV.find(*uit) != enetsV.cend() ) { return false; }
            }
            return true;
        }

        TreePtrT readNewickTree( const std::string& treeName ) {
            unique_ptr<bpp::Nhx> newickReader( new bpp::Nhx(true) ); //No comment allowed!
            //newickReader->enableExtendedBootstrapProperty("name");
            try {
                TreePtrT tree( newickReader->read(treeName) ); // Tree in file
                for ( auto pn : tree->getNodePropertyNames( tree->getRootId()) ) {
                    std::cerr << "property : " << pn << "\t";
                    std::cerr << "value : " << dynamic_cast<bpp::BppString*>(tree->getNodeProperty(tree->getRootId(), pn))->toSTL() << "\n";
                }

                return tree;
            } catch(std::exception& e) {
                cerr << "Caught Exception: [" << e.what() << "]\n";
                abort();
            }
        }


        const std::string getName( TreePtrT& t, int nid) {
            if ( t->hasNodeProperty(nid,"GN") ) {
                auto name = dynamic_cast<bpp::BppString*>( t->getNodeProperty(nid,"GN"))->toSTL();
                return name;
            } else if (t->hasNodeName(nid)) {
                return t->getNodeName(nid);
            }
        }

        void labelTree( TreePtrT& tree ) {
            for ( auto nid : tree->getNodesId() ) {
                tree->setNodeName(nid, getName(tree, nid) );
            }
        }

        string getExtantNetwork(const string& s) {
            return ( s.find("LOST") != string::npos ) ? "LOST" : s.substr( s.find_last_of('_') );
        }

        string getSpeciesName(const TreePtrT& t, int nid) {
            return dynamic_cast<bpp::BppString*>( t->getNodeProperty(nid,"S") )->toSTL();
        }

        bool isDescendantSpecies(const TreeInfo& ti, int u, int v)  {

            if ( ti.tree->isLeaf(v) ) { return false; }
            auto sonIDs = ti.tree->getSonsId(v);

            auto& subspecLV = ti.subspec.find(sonIDs[0])->second;
            auto& subspecRV = ti.subspec.find(sonIDs[1])->second;
            
            auto specU = getSpeciesName(ti.tree, u);

            auto res = ( (subspecLV.find( specU ) != subspecLV.end()) or 
                         (subspecRV.find( specU ) != subspecRV.end()) );
            return res;

            /*
            auto& enetsU = ti.enets.find(u)->second;
            auto& enetsV = ti.enets.find(v)->second;

            // If u has more descendant species than v, then it can't be a descendant of v
            if (enetsU.size() > enetsV.size()) { return false; }

            // Every species below u must be represented in v
            for (auto uit = enetsU.cbegin(); uit != enetsU.cend(); ++uit) {
                if ( enetsV.find(*uit) == enetsV.cend() ) { return false; }
            }
            return true;
            */
        }

        bool sameSpecies(const TreePtrT& t, int u, int v) {
            return getSpeciesName(t, u) == getSpeciesName(t, v);
        }

        void prepareTree( TreePtrT& t, TreeInfo& ti, int nid ) {
            // if the current node is not the root
            if ( nid != t->getRootId() ) {
                auto fid = t->getFatherId(nid);
                auto parentDeathT = get<1>(ti.extantInterval[fid]);
                assert ( ti.extantInterval.find(fid) != ti.extantInterval.end() );
                double fdist = 0.0;
                if ( t->isLeaf(nid) ) {
                    fdist = std::numeric_limits<double>::infinity();
                } else if ( t->hasDistanceToFather(nid) ) {
                    fdist = t->getDistanceToFather(nid);
                }
                ti.extantInterval[ nid ] = make_tuple( parentDeathT, parentDeathT + fdist );
            }

            if (t->isLeaf(nid)) {
                ti.leaves[nid] = {nid};
                ti.subnodes[nid] = {nid};
                string enet = getSpeciesName(t,nid); //getExtantNetwork(getName(t,nid));

                // Skip lost nodes
                ti.enets[nid] = {enet};
                ti.subspec[nid] = {enet};
                
                // Consider lost nodes
                //ti.enets[nid] = {enet};
            } else {
                ti.leaves[nid] = {};
                ti.subnodes[nid] = { nid };
                ti.enets[nid] = unordered_set<string>();
                ti.subspec[nid] = { getSpeciesName(t,nid) };
                //ti.enets[nid] = { getSpeciesName(t,nid) };


                for ( auto cid : t->getSonsId(nid) ) {
                    prepareTree( t, ti, cid );
                }

                for ( auto cid : t->getSonsId(nid) ) {
                    for ( auto l : ti.leaves[cid] ) { ti.leaves[nid].insert(l); }
                    for ( auto l : ti.subnodes[cid] ) { ti.subnodes[nid].insert(l); }
                    for ( auto l : ti.enets[cid] ) { ti.enets[nid].insert(l); }
                    for ( auto l : ti.subspec[cid] ) { ti.subspec[nid].insert(l); }
                }
            }
        }




    }

}

template bool Utils::appendNext<MultiOpt::dvsT, MultiOpt::QueueCmp<MultiOpt::dvsT>>( double, const vector<size_t>&, const vector<size_t>& , vector<MultiOpt::dvsT>& , MultiOpt::QueueCmp<MultiOpt::dvsT>& , std::function< double(const vector<size_t>&) >& );


typedef boost::heap::pairing_heap<MultiOpt::edvsT, boost::heap::compare<MultiOpt::QueueCmp<MultiOpt::edvsT>>> heapT;
template bool Utils::appendNextWithEdge<heapT>( const size_t&,
                                                const vector<size_t>&,
                                                const vector<size_t>& ,
                                                heapT&,
                                                std::function< double(const size_t&, const vector<size_t>&) >&);
/*
template bool Utils::appendNextWithEdgeOrig<heapT>( const size_t&,
                                                const vector<size_t>&,
                                                const vector<size_t>& ,
                                                heapT&,
                                                std::function< double(const size_t&, const vector<size_t>&) >&);
*/
