/*!
 *  \file       PHTpcTrackerUtil.cc
 *  \brief      
 *  \author     Dmitry Arkhipkin <arkhipkin@gmail.com>
 */

#include "PHTpcTrackerUtil.h"

#include <trackbase/TrkrCluster.h>
#include <trackbase/TrkrClusterContainer.h>  // for TrkrClusterContainer
#include <trackbase/TrkrDefs.h>

#include <phool/PHLog.h>

#include <cstdint>  // for uint64_t
#include <cstring>
#include <map>      // for _Rb_tree_const_iterator
#include <utility>  // for pair

namespace PHTpcTrackerUtil
{
  template< class T> inline constexpr T square( const T& x ) { return x*x; }
  
  std::vector<std::vector<double> > convert_clusters_to_hits(TrkrClusterContainer* cluster_map)
  {
    //***** convert clusters to kdhits
    std::vector<std::vector<double> > kdhits;
    if (!cluster_map)
    {
      LOG_WARN("tracking.PHTpcTrackerUtil.convert_clusters_to_hits") << "cluster map is not provided";
      return kdhits;
    }
    for(const auto& hitsetkey:cluster_map->getHitSetKeys(TrkrDefs::TrkrId::tpcId))
    {
      std::string separator = "";
      auto range = cluster_map->getClusters(hitsetkey);
      for( auto it = range.first; it != range.second; ++it )
	{
   TrkrDefs::cluskey cluskey = it->first;
	  TrkrCluster* cluster = it->second;
	  if ((square( cluster->getPosition(0)) +
	       square( cluster->getPosition(1)) +
	       square( cluster->getPosition(2))) > (25.0 * 25.0))
	    {
	      std::vector<double> kdhit(4);
	      kdhit[0] = cluster->getPosition(0);
	      kdhit[1] = cluster->getPosition(1);
	      kdhit[2] = cluster->getPosition(2);
	      uint64_t key = cluskey;
	      std::memcpy(&kdhit[3], &key, sizeof(key));
	      
	      //	HINT: way to get original uint64_t value from double:
	      //
	      //			LOG_DEBUG("tracking.PHTpcTrackerUtil.convert_clusters_to_hits")
	      //				<< "orig: " << cluster->getClusKey() << ", readback: " << (*((int64_t*)&kdhit[3]));
	      
	      kdhits.push_back(kdhit);
	    }
	}
    }
    return kdhits;
  }

}  // namespace PHTpcTrackerUtil
