#include "PHMicromegasTpcTrackMatching.h"

//#include "PHTrackPropagating.h"     // for PHTrackPropagating

#include <g4detectors/PHG4CylinderGeomContainer.h>

#include <micromegas/CylinderGeomMicromegas.h>
#include <micromegas/MicromegasDefs.h>

/// Tracking includes
#include <trackbase/TrackFitUtils.h>
#include <trackbase/TrkrCluster.h>            // for TrkrCluster
#include <trackbase/TrkrClusterv3.h>            // for TrkrCluster
#include <trackbase/TrkrDefs.h>               // for cluskey, getLayer, TrkrId
#include <trackbase/TrkrClusterContainer.h>
#include <trackbase/TrkrClusterIterationMapv1.h>

#include <trackbase_historic/TrackSeed.h>     
#include <trackbase_historic/TrackSeedContainer.h>

#include <tpc/TpcDistortionCorrectionContainer.h>
#include <tpc/TpcDefs.h>

#include <trackbase_historic/ActsTransformations.h>

#include <fun4all/Fun4AllReturnCodes.h>
#include <fun4all/SubsysReco.h>                // for SubsysReco

#include <phool/getClass.h>
#include <phool/phool.h>

#include <TF1.h>
#include <TVector3.h>

#include <array>
#include <cmath>                              // for sqrt, std::abs, atan2, cos
#include <iostream>                           // for operator<<, basic_ostream
#include <map>                                // for map
#include <set>                                // for _Rb_tree_const_iterator
#include <utility>                            // for pair, make_pair

namespace
{

  //! convenience square method
  template<class T>
    inline constexpr T square( const T& x ) { return x*x; }

  //! get radius from x and y
  template<class T>
    inline constexpr T get_r( const T& x, const T& y ) { return std::sqrt( square(x) + square(y) ); }

  /// calculate intersection from circle to line, in 2d. return true on success
  /**
   * circle is defined as (x-xc)**2 + (y-yc)**2 = r**2
   * line is defined as nx(x-x0) + ny(y-y0) = 0
   * to solve we substitute y by y0 - nx/ny*(x-x0) in the circle equation and solve the 2nd order polynom
   * there is the extra complication that ny can be 0 (vertical line) to prevent this, we multiply all terms of the polynom by ny**2
   * and account for this special case when calculating x from y
   */
  bool circle_line_intersection(
    double r, double xc, double yc,
    double x0, double y0, double nx, double ny,
    double &xplus, double &yplus, double &xminus, double &yminus)
  {
    if( ny == 0 )
    {
      // vertical lines are defined by ny=0 and x = x0
      xplus = xminus = x0;

      // calculate y accordingly
      const double delta = square(r) - square(x0-xc);
      if( delta < 0 ) return false;

      const double sqdelta = std::sqrt( delta );
      yplus = yc + sqdelta;
      yminus = yc - sqdelta;

    } else {

      const double a = square(nx) + square(ny);
      const double b = -2.*( square(ny)*xc + square(nx)*x0 + nx*ny*(y0-yc) );
      const double c = square(ny)*(square(xc)-square(r)) + square(ny*(y0-yc)+nx*x0);
      const double delta = square(b) - 4.*a*c;
      if( delta < 0 ) return false;

      const double sqdelta = std::sqrt( delta );
      xplus = (-b + sqdelta)/(2.*a);
      xminus = (-b - sqdelta)/(2.*a);

      yplus = y0 -(nx/ny)*(xplus-x0);
      yminus = y0 - (nx/ny)*(xminus-x0);

    }

    return true;

  }

  // streamer of TVector3
  [[maybe_unused]] inline std::ostream& operator << (std::ostream& out, const TVector3& vector )
  {
    out << "( " << vector.x() << "," << vector.y() << "," << vector.z() << ")";
    return out;
  }

}

//____________________________________________________________________________..
PHMicromegasTpcTrackMatching::PHMicromegasTpcTrackMatching(const std::string &name):
 SubsysReco(name)
{}

//____________________________________________________________________________..
int PHMicromegasTpcTrackMatching::InitRun(PHCompositeNode *topNode)
{

  std::cout << std::endl << PHWHERE
	    << " rphi_search_win inner layer " << _rphi_search_win[0]
	    << " z_search_win inner layer " << _z_search_win[0]
	    << " rphi_search_win outer layer " << _rphi_search_win[1]
	    << " z_search_win outer layer " << _z_search_win[1]
	    << std::endl;

  // load micromegas geometry
  _geomContainerMicromegas = findNode::getClass<PHG4CylinderGeomContainer>(topNode, "CYLINDERGEOM_MICROMEGAS_FULL" );
  if(!_geomContainerMicromegas)
  {
    std::cout << PHWHERE << "Could not find CYLINDERGEOM_MICROMEGAS_FULL." << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }

  // ensures there are at least two micromegas layers
  if( _geomContainerMicromegas->get_NLayers() != _n_mm_layers )
  {
    std::cout << PHWHERE << "Inconsistent number of micromegas layers." << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }

  // get first micromegas layer
  _min_mm_layer = static_cast<CylinderGeomMicromegas*>(_geomContainerMicromegas->GetFirstLayerGeom())->get_layer();

  fdrphi = new TF1("fdrphi", "[0] + [1]*std::abs(x)");
  fdrphi->SetParameter(0, _par0 *_collision_rate / _reference_collision_rate);
  fdrphi->SetParameter(1, _par1);

  // base class setup
 
  int ret = GetNodes(topNode);
  if (ret != Fun4AllReturnCodes::EVENT_OK) return ret;

  return ret;
}

//____________________________________________________________________________..
int PHMicromegasTpcTrackMatching::process_event(PHCompositeNode* topNode)
{

  if(_n_iteration >0)
  {
    _iteration_map = findNode::getClass<TrkrClusterIterationMapv1>(topNode, "CLUSTER_ITERATION_MAP");
    if (!_iteration_map)
    {
      std::cerr << PHWHERE << "Cluster Iteration Map missing, aborting." << std::endl;
      return Fun4AllReturnCodes::ABORTEVENT;
    }
  }
  // _track_map contains the TPC seed track stubs
  // We will add the micromegas cluster to the TPC tracks already on the node tree
  // We will have to expand the number of tracks whenever we find multiple matches to the silicon

  _event++;

  if(Verbosity() > 0)
    std::cout << PHWHERE << " Event " << _event << " TPC track map size " << _track_map->size() << std::endl;
  
  // We remember the original size of the TPC track map here
  const unsigned int original_track_map_lastkey = _track_map->empty() ? 0 : _track_map->size() - 1;

  // loop over the original TPC tracks
  for (unsigned int tpcID = 0; 
       tpcID != _track_map->size(); ++tpcID)
  {
    // we may add tracks to the map, so we stop at the last original track
    if(tpcID > original_track_map_lastkey)  break;

    auto tracklet_tpc = _track_map->get(tpcID);
    if(!tracklet_tpc) { continue; }

    short int crossing = tracklet_tpc->get_crossing();

    if (crossing == SHRT_MAX) continue;   // not matched to silicon, skip it


    if (Verbosity() >= 1)
    {
      std::cout << std::endl
        << __LINE__
        << ": Processing seed itrack: " << tpcID
        << ": crossing: " << crossing
        << ": nhits: " << tracklet_tpc-> size_cluster_keys()
        << ": Total tracks: " << _track_map->size()
        << ": phi: " << tracklet_tpc->get_phi(_cluster_map,_surfmaps,_tGeometry)
        << std::endl;
    }

    // Get the outermost TPC clusters for this tracklet
    std::map<unsigned int, TrkrCluster*> outer_clusters;
    std::vector<TrkrCluster*> clusters;
    std::vector<Acts::Vector3> clusGlobPos;
    ActsTransformations transformer;

    for (SvtxTrack::ConstClusterKeyIter key_iter = tracklet_tpc->begin_cluster_keys(); key_iter != tracklet_tpc->end_cluster_keys(); ++key_iter)
    {
      TrkrDefs::cluskey cluster_key = *key_iter;
      unsigned int layer = TrkrDefs::getLayer(cluster_key);

      if(layer < _min_tpc_layer) continue;
      if(layer >= _min_mm_layer) continue;

      // get the cluster
      TrkrCluster *tpc_clus =  _cluster_map->findCluster(cluster_key);
      if(!tpc_clus) continue;

      outer_clusters.insert(std::make_pair(layer, tpc_clus));
      clusters.push_back(tpc_clus);
      // make necessary corrections to the global position
      unsigned int side = TpcDefs::getSide(cluster_key);
      const Acts::Vector3 global = getGlobalPosition(cluster_key, tpc_clus, crossing, side);
      clusGlobPos.push_back(global);
      
      if(Verbosity() > 10)
      {
	auto global_raw = transformer.getGlobalPosition(cluster_key, tpc_clus, _surfmaps, _tGeometry);
        std::cout
          << "  TPC cluster key " << cluster_key << " in layer " << layer << " side " << side << " crossing " << crossing
	  << " with local position " << tpc_clus->getLocalX()  << "  " << tpc_clus->getLocalY() << std::endl;
	std::cout << " raw global position " << global_raw[0] << " " << global_raw[1] << " " << global_raw[2]
	  << " corrected global position " << global[0] << " " << global[1] << " " << global[2]
	  << std::endl;
      }
    }

    // need at least 3 clusters to fit a circle
    if(outer_clusters.size() < 3)
    {
      if(Verbosity() > 3) std::cout << PHWHERE << "  -- skip this tpc tracklet, not enough outer clusters " << std::endl;
      continue;  // skip to the next TPC tracklet
    }

    // fit a circle to the clusters
    const auto [R, X0, Y0] = TrackFitUtils::circle_fit_by_taubin( clusGlobPos );
    if(Verbosity() > 10) std::cout << " Fitted circle has R " << R << " X0 " << X0 << " Y0 " << Y0 << std::endl;

    // toss tracks for which the fitted circle could not have come from the vertex
    if(R < 40.0) continue;

    // get the straight line representing the z trajectory in the form of z vs radius
    const auto [A, B] = TrackFitUtils::line_fit( clusGlobPos );
    if(Verbosity() > 10) std::cout << " Fitted line has A " << A << " B " << B << std::endl;

    // loop over micromegas layer
    for(unsigned int imm = 0; imm < _n_mm_layers; ++imm)
    {

      // get micromegas geometry object
      const unsigned int layer = _min_mm_layer + imm;
      const auto layergeom = static_cast<CylinderGeomMicromegas*>(_geomContainerMicromegas->GetLayerGeom(layer));
      const auto layer_radius = layergeom->get_radius();

      // method to find where fitted circle intersects this layer
      auto [xplus, yplus, xminus, yminus] = TrackFitUtils::circle_circle_intersection(	layer_radius, R, X0, Y0 );

      // finds the intersection of the fitted circle with the micromegas layer
      if( !std::isfinite(xplus) )
      {
        if(Verbosity() > 10)
        {
          std::cout << PHWHERE << " circle/circle intersection calculation failed, skip this case" << std::endl;
          std::cout << PHWHERE << " mm_radius " << layer_radius << " fitted R " << R << " fitted X0 " << X0 << " fitted Y0 " << Y0 << std::endl;
        }

        continue;
      }

      // we can figure out which solution is correct based on the last cluster position in the TPC
      const double last_clus_phi = std::atan2(clusGlobPos.back()(1), clusGlobPos.back()(0));
      double phi_plus = std::atan2(yplus, xplus);
      double phi_minus = std::atan2(yminus, xminus);

      // calculate z
      double r = layer_radius;
      double z = B + A*r;

      // select the angle that is the closest to last cluster
      // store phi, apply coarse space charge corrections in calibration mode
      double phi = std::abs(last_clus_phi - phi_plus) < std::abs(last_clus_phi - phi_minus) ? phi_plus:phi_minus;
      if(_sc_calib_mode) phi -= fdrphi->Eval(z)/r;

      // create cylinder intersection point in world coordinates
      const TVector3 world_intersection_cylindrical( r*std::cos(phi), r*std::sin(phi), z );

      // find matching tile
      int tileid = layergeom->find_tile_cylindrical( world_intersection_cylindrical );
      if( tileid < 0 ) continue;

      // get tile coordinates
      const auto tile_center_phi = layergeom->get_center_phi( tileid );
      const double x0 = r*std::cos( tile_center_phi );
      const double y0 = r*std::sin( tile_center_phi );

      const double nx = x0;
      const double ny = y0;

      // calculate intersection to tile
      if( !circle_line_intersection( R, X0, Y0, x0, y0, nx, ny, xplus, yplus, xminus, yminus) )
      {
        if(Verbosity() > 10)
        { std::cout << PHWHERE << "circle_line_intersection - failed" << std::endl; }
        continue;
      }

      // select again angle closest to last cluster
      phi_plus = std::atan2(yplus, xplus);
      phi_minus = std::atan2(yminus, xminus);
      const bool is_plus = (std::abs(last_clus_phi - phi_plus) < std::abs(last_clus_phi - phi_minus));

      // calculate x, y and z
      const double x = (is_plus ? xplus:xminus);
      const double y = (is_plus ? yplus:yminus);
      r = get_r( x, y );
      z = B + A*r;

      /*
       * create planar intersection point in world coordinates
       * this is the position to be compared to the clusters
       */
      const TVector3 world_intersection_planar( x, y, z );

      // convert to tile local reference frame, apply SC correction
      TVector3 local_intersection_planar = layergeom->get_local_from_world_coords( tileid, world_intersection_planar );
      if(_sc_calib_mode)
      {
        /*
         * apply SC correction to the local intersection,
         * to make sure that the corrected intersection is still in the micromegas plane
         * in local tile coordinates, the rphi direction, to which the correction is applied, corresponds to the x direction
         */
        local_intersection_planar.SetX( local_intersection_planar.x() - fdrphi->Eval(z) );
      }

      // store segmentation type
      const auto segmentation_type = layergeom->get_segmentation_type();

      // generate tilesetid and get corresponding clusters
      const auto tilesetid = MicromegasDefs::genHitSetKey(layer, segmentation_type, tileid);
      const auto mm_clusrange = _cluster_map->getClusters(tilesetid);

      // convert to tile local coordinate and compare
      for(auto clusiter = mm_clusrange.first; clusiter != mm_clusrange.second; ++clusiter)
      {
	TrkrDefs::cluskey ckey = clusiter->first;
	if(_iteration_map != NULL ){
	  if( _iteration_map->getIteration(ckey) > 0) 
	    continue; // skip hits used in a previous iteration
	}
        // store cluster and key
        const auto& [key, cluster] = *clusiter;
        const auto glob = transformer.getGlobalPosition(key, cluster,_surfmaps,_tGeometry);
        const TVector3 world_cluster(glob(0), glob(1), glob(2));
        const TVector3 local_cluster = layergeom->get_local_from_world_coords( tileid, world_cluster );

        // compute residuals and store
        /* in local tile coordinate, x is along rphi, and z is along z) */
        const double drphi = local_intersection_planar.x() - local_cluster.x();
        const double dz = local_intersection_planar.z() - local_cluster.z();

        // compare to cuts and add to track if matching
        if( std::abs(drphi) < _rphi_search_win[imm] && std::abs(dz) < _z_search_win[imm] )
        {
          tracklet_tpc->insert_cluster_key(key);

          // prints out a line that can be grep-ed from the output file to feed to a display macro
          if( _test_windows )
          {
            // cluster rphi and z
            const double mm_clus_rphi = get_r( glob(0), glob(1) ) * std::atan2( glob(1),  glob(0) );
            const double mm_clus_z = glob(2);

            // projection phi and z, without correction
            const double rphi_proj = get_r( world_intersection_planar.x(), world_intersection_planar.y() ) * std::atan2( world_intersection_planar.y(), world_intersection_planar.x() );
            const double z_proj = world_intersection_planar.z();

            /*
             * Note: drphi and dz might not match the difference of the rphi and z quoted values. This is because
             * 1/ drphi and dz are actually calculated in Tile's local reference frame, not in world coordinates
             * 2/ drphi also includes SC distortion correction, which the world coordinates don't
             */
            std::cout
              << "  Try_mms: " << (int) layer
              << " drphi " << drphi
              << " dz " << dz
              << " mm_clus_rphi " << mm_clus_rphi << " mm_clus_z " << mm_clus_z
              << " rphi_proj " <<  rphi_proj << " z_proj " << z_proj
              << std::endl;
          }
        }

      } // end loop over clusters

    } // end loop over Micromegas layers

    if(Verbosity() > 3)
    { tracklet_tpc->identify(); }

  }
  /*
  // loop over all tracks and copy the silicon clusters to the corrected cluster map
  if(_corrected_cluster_map)
  { copyMicromegasClustersToCorrectedMap(); }
  */

  if(Verbosity() > 0)
  { std::cout << " Final track map size " << _track_map->size() << std::endl; }

  return Fun4AllReturnCodes::EVENT_OK;
}

//_________________________________________________________________________________________________
int PHMicromegasTpcTrackMatching::End(PHCompositeNode*)
{ return Fun4AllReturnCodes::EVENT_OK; }

//_________________________________________________________________________________________________
int  PHMicromegasTpcTrackMatching::GetNodes(PHCompositeNode* topNode)
{
  /*
  // tpc-distortion-corrected clusters
  _corrected_cluster_map = findNode::getClass<TrkrClusterContainer>(topNode,"CORRECTED_TRKR_CLUSTER");
  if(_corrected_cluster_map)
    {
      std::cout << " Found CORRECTED_TRKR_CLUSTER node " << std::endl;
    }
  */
    
  // all clusters
  if(_use_truth_clusters)
    _cluster_map = findNode::getClass<TrkrClusterContainer>(topNode, "TRKR_CLUSTER_TRUTH");
  else
    {
      _cluster_map = findNode::getClass<TrkrClusterContainer>(topNode, "TRKR_CLUSTER");
    }

  if (!_cluster_map)
  {
    std:: cerr << PHWHERE << " ERROR: Can't find node TRKR_CLUSTER" << std::endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }
    

  _tGeometry = findNode::getClass<ActsTrackingGeometry>(topNode, "ActsTrackingGeometry");
  if(!_tGeometry)
    {
      std::cerr << PHWHERE << "No acts tracking geometry, can't continue."
		<< std::endl;
      return Fun4AllReturnCodes::ABORTEVENT;
    }

  _surfmaps = findNode::getClass<ActsSurfaceMaps>(topNode, "ActsSurfaceMaps");
  if(!_surfmaps)
    {
      std::cerr << PHWHERE << "No acts surface son node tree, exiting." 
		<< std::endl;
      return Fun4AllReturnCodes::ABORTEVENT;
    }

  _track_map = findNode::getClass<TrackSeedContainer>(topNode, _track_map_name);
  if (!_track_map)
  {
    std::cerr << PHWHERE << " ERROR: Can't find "<< _track_map_name.c_str() << std::endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

  // micromegas geometry
  _geomContainerMicromegas = findNode::getClass<PHG4CylinderGeomContainer>(topNode, "CYLINDERGEOM_MICROMEGAS_FULL" );
  if(!_geomContainerMicromegas)
  {
    std::cout << PHWHERE << "Could not find CYLINDERGEOM_MICROMEGAS_FULL." << std::endl;
    return Fun4AllReturnCodes::ABORTEVENT;
  }

 // tpc distortion correction
  _dcc = findNode::getClass<TpcDistortionCorrectionContainer>(topNode,"TpcDistortionCorrectionContainerStatic");
  if( _dcc )
  { std::cout << "PHMicromegasTpcTrackMatching::get_Nodes  - found static TPC distortion correction container" << std::endl; }

  return Fun4AllReturnCodes::EVENT_OK;
}

/*
void PHMicromegasTpcTrackMatching::copyMicromegasClustersToCorrectedMap( )
{
  // loop over final track map, copy micromegas clusters to corrected cluster map
  for( auto track_iter = _track_map->begin(); track_iter != _track_map->end(); ++track_iter )
  {    
    TrackSeed* track = *track_iter;
    // loop over associated clusters to get keys for micromegas cluster
    for(auto iter = track->begin_cluster_keys(); iter != track->end_cluster_keys(); ++iter)
    {
      TrkrDefs::cluskey cluster_key = *iter;
      const unsigned int trkrid = TrkrDefs::getTrkrId(cluster_key);
      if(trkrid == TrkrDefs::micromegasId)
	    {

        // check if clusters has not been inserted already
        if( _corrected_cluster_map->findCluster( cluster_key ) ) continue;
        
        // get cluster from original map
        auto cluster =  _cluster_map->findCluster(cluster_key);
        if( !cluster ) continue;

        // create a new cluster and copy from source
        auto newclus = new TrkrClusterv3;
        newclus->CopyFrom( cluster );

        // insert in corrected map
        _corrected_cluster_map->addClusterSpecifyKey(cluster_key, newclus);

	    }
    }
  }
}
*/

 Acts::Vector3 PHMicromegasTpcTrackMatching::getGlobalPosition( TrkrDefs::cluskey key, TrkrCluster* cluster, short int crossing, unsigned int side)
{
  // get global position from Acts transform
  ActsTransformations transformer;
  auto globalpos = transformer.getGlobalPosition(key, cluster,
    _surfmaps,
    _tGeometry);

  // ADF: for streaming mode, will need a crossing z correction here
  float z = _clusterCrossingCorrection.correctZ(globalpos[2], side, crossing);
  globalpos[2] = z;

  // check if TPC distortion correction are in place and apply
  if(_dcc) { globalpos = _distortionCorrection.get_corrected_position( globalpos, _dcc ); }

  return globalpos;
}
  


