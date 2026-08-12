
#include "TriggeredTrackSkimmer.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/PHCompositeNode.h>
#include <phool/getClass.h>

#include <trackbase_historic/SvtxTrack.h>
#include <trackbase_historic/SvtxTrackMap.h>
#include <trackbase_historic/SvtxTrackMap_v2.h>
#include <trackbase_historic/TrackSeedContainer.h>
#include <trackbase_historic/TrackSeedContainer_v1.h>
#include <trackbase_historic/TrackSeed.h>

#include <globalvertex/SvtxVertex.h>
#include <globalvertex/SvtxVertexMap.h>
#include <globalvertex/SvtxVertexMap_v1.h>


//____________________________________________________________________________..
TriggeredTrackSkimmer::TriggeredTrackSkimmer(const std::string &name):
 SubsysReco(name)
{
  std::cout << "TriggeredTrackSkimmer::TriggeredTrackSkimmer(const std::string &name) Calling ctor" << std::endl;
}

//____________________________________________________________________________..
TriggeredTrackSkimmer::~TriggeredTrackSkimmer()
{
  std::cout << "TriggeredTrackSkimmer::~TriggeredTrackSkimmer() Calling dtor" << std::endl;
}

//____________________________________________________________________________..
int TriggeredTrackSkimmer::Init([[maybe_unused]] PHCompositeNode *topNode)
{
  //std::cout << "TriggeredTrackSkimmer::Init(PHCompositeNode *topNode) Initializing" << std::endl;
  
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TriggeredTrackSkimmer::InitRun([[maybe_unused]] PHCompositeNode *topNode)
{
  std::cout << "TriggeredTrackSkimmer::InitRun(PHCompositeNode *topNode) Initializing for Run XXX" << std::endl;
  CreateNode(topNode);
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TriggeredTrackSkimmer::process_event([[maybe_unused]] PHCompositeNode *topNode)
{
  //std::cout << "TriggeredTrackSkimmer::process_event(PHCompositeNode *topNode) Processing Event" << std::endl;

  
  //get tracks and check crossing
  m_trackMap = findNode::getClass<SvtxTrackMap>(topNode, m_trackMapName);
  if (!m_trackMap)
  {
    std::cerr << Name() << ": required node is missing:\n"
              << "  " << m_trackMapName << std::endl;
  }

  //get output track and seed containers
  SvtxTrackMap *triggertracks = findNode::getClass<SvtxTrackMap>(topNode, m_triggertrackNode);
  TrackSeedContainer *triggersseeds = findNode::getClass<TrackSeedContainer>(topNode, m_triggersseedNode);
  TrackSeedContainer *triggertseeds = findNode::getClass<TrackSeedContainer>(topNode, m_triggertseedNode);

  SvtxTrackMap *streamtracks = findNode::getClass<SvtxTrackMap>(topNode, m_streamtrackNode);
  TrackSeedContainer *streamsseeds = findNode::getClass<TrackSeedContainer>(topNode, m_streamsseedNode);
  TrackSeedContainer *streamtseeds = findNode::getClass<TrackSeedContainer>(topNode, m_streamtseedNode);
  
  m_siliconSeedMap = findNode::getClass<TrackSeedContainer>(topNode, m_siliconSeedMapName);
  if (!m_siliconSeedMap)
  {
    std::cerr << Name() << ": required node is missing:\n"
              << "  " << m_siliconSeedMapName << std::endl;
  }

  m_TPCSeedMap = findNode::getClass<TrackSeedContainer>(topNode, m_TPCSeedMapName);
  if (!m_TPCSeedMap)
  {
    std::cerr << Name() << ": required node is missing:\n"
              << "  " << m_TPCSeedMapName << std::endl;
  }


  for (const auto &[trackKey, track] : *m_trackMap)
  {
   int trackCrossing = track->get_crossing();
   if(trackCrossing == 0)
   {
     triggertracks->insert(track);

     TrackSeed *sseed = track->get_silicon_seed();
     if(sseed)
     {
       triggersseeds->insert(sseed);
     }

     TrackSeed *tseed = track->get_tpc_seed();
     if(tseed)
     {
       triggertseeds->insert(tseed);
     }
               
   }
   else
   {
     streamtracks->insert(track);

     TrackSeed *sseed = track->get_silicon_seed();
     if(sseed)
     {
       streamsseeds->insert(sseed);
     }

     TrackSeed *tseed = track->get_tpc_seed();
     if(tseed)
     {
       streamtseeds->insert(tseed);
     }
   }
  }

  //get vertices and crossing
  m_vertexMap = findNode::getClass<SvtxVertexMap>(topNode, m_vertexMapName);
  if (!m_vertexMap)
  {
    std::cerr << Name() << ": required node is missing:\n"
              << "  " << m_vertexMapName << std::endl;
  }
  SvtxVertexMap *triggervertex = findNode::getClass<SvtxVertexMap>(topNode, m_triggervertexNode);
  SvtxVertexMap *streamvertex = findNode::getClass<SvtxVertexMap>(topNode, m_streamvertexNode);

  for (const auto &[vertexKey, vertex] : *m_vertexMap)
  {
    if(!vertex) continue; 
    int vertexCrossing = vertex->get_beam_crossing();
    if(vertexCrossing == 0)
    {
      triggervertex->insert_clone(vertex);
    }
    else
    {
      streamvertex->insert_clone(vertex);
    }
  }
   
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TriggeredTrackSkimmer::ResetEvent([[maybe_unused]] PHCompositeNode *topNode)
{
  //std::cout << "TriggeredTrackSkimmer::ResetEvent(PHCompositeNode *topNode) Resetting internal structures, prepare for next event" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TriggeredTrackSkimmer::EndRun(const int runnumber)
{
  std::cout << "TriggeredTrackSkimmer::EndRun(const int runnumber) Ending Run for Run " << runnumber << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TriggeredTrackSkimmer::End([[maybe_unused]] PHCompositeNode *topNode)
{
  std::cout << "TriggeredTrackSkimmer::End(PHCompositeNode *topNode) This is the End..." << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int TriggeredTrackSkimmer::Reset([[maybe_unused]] PHCompositeNode *topNode)
{
 std::cout << "TriggeredTrackSkimmer::Reset(PHCompositeNode *topNode) being Reset" << std::endl;
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
void TriggeredTrackSkimmer::Print(const std::string &what) const
{
  std::cout << "TriggeredTrackSkimmer::Print(const std::string &what) const Printing info for " << what << std::endl;
}

int TriggeredTrackSkimmer::CreateNode(PHCompositeNode *topNode)
{
  // Check nodes.
  PHNodeIterator iter(topNode);
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
  {
    std::cout << PHWHERE << "TriggeredTrackSkimmer::CreateNode : DST Node missing, aborting!" << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }
  PHCompositeNode *svtxnode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "SVTX"));
  if (!svtxnode)
  {
    std::cout << PHWHERE << "TriggeredTrackSkimmer::CreateNode : SVTX node missing, aborting!" << std::endl;
    return Fun4AllReturnCodes::ABORTRUN;
  }


  // Create skimmed nodes 
  CreateTrackNode(topNode, svtxnode, m_triggertrackNode);
  CreateSeedNode(topNode, svtxnode, m_triggersseedNode);
  CreateSeedNode(topNode, svtxnode, m_triggertseedNode);
  CreateVertexNode(topNode, svtxnode, m_triggervertexNode);

  CreateTrackNode(topNode, svtxnode, m_streamtrackNode);
  CreateSeedNode(topNode, svtxnode, m_streamsseedNode);
  CreateSeedNode(topNode, svtxnode, m_streamtseedNode);
  CreateVertexNode(topNode, svtxnode, m_streamvertexNode);
  
  return Fun4AllReturnCodes::EVENT_OK;
}

int TriggeredTrackSkimmer::CreateSeedNode(PHCompositeNode *topNode, PHCompositeNode *svtxnode, std::string nodename = "")
{
  TrackSeedContainer *test_sseeds = findNode::getClass<TrackSeedContainer>(topNode, nodename);
  if (!test_sseeds)
  {
    TrackSeedContainer *triggersseeds = new TrackSeedContainer_v1();
    PHIODataNode<PHObject> *triggersseednode;
    if (Verbosity() > 0)
    {
      std::cout << "TriggeredTrackSkimmer::CreateNode : creating " << nodename << std::endl;
    }
    triggersseednode = new PHIODataNode<PHObject>(triggersseeds, nodename, "PHObject");
    svtxnode->addNode(triggersseednode);
  }
  else
  {
    std::cout << "TriggeredTrackSkimmer::CreateNode : " << nodename << " already exists! " << std::endl;
  }
  return Fun4AllReturnCodes::EVENT_OK;
}

int TriggeredTrackSkimmer::CreateTrackNode(PHCompositeNode *topNode, PHCompositeNode *svtxnode, std::string nodename = "")
{
  SvtxTrackMap *test_tracks = findNode::getClass<SvtxTrackMap>(topNode, nodename);
  if (!test_tracks)
  {
    SvtxTrackMap *triggertracks = new SvtxTrackMap_v2();
    PHIODataNode<PHObject> *triggertracknode;
    if (Verbosity() > 0)
    {
      std::cout << "TriggeredTrackSkimmer::CreateNode : creating " << nodename << std::endl;
    }
    triggertracknode = new PHIODataNode<PHObject>(triggertracks, nodename, "PHObject");
    svtxnode->addNode(triggertracknode);
  }
  else
  {
    std::cout << "TriggeredTrackSkimmer::CreateNode : " << nodename << " already exists! " << std::endl;
  }
  return Fun4AllReturnCodes::EVENT_OK;
}

int TriggeredTrackSkimmer::CreateVertexNode(PHCompositeNode *topNode, PHCompositeNode *svtxnode, std::string nodename = "")
{
 SvtxVertexMap *test_vertex = findNode::getClass<SvtxVertexMap>(topNode, nodename);
  if (!test_vertex)
  {
    SvtxVertexMap *triggervertex = new SvtxVertexMap_v1();
    PHIODataNode<PHObject> *triggervertexnode;
    if (Verbosity() > 0)
    {
      std::cout << "TriggeredVertexSkimmer::CreateNode : creating " << nodename << std::endl;
    }
    triggervertexnode = new PHIODataNode<PHObject>(triggervertex, nodename, "PHObject");
    svtxnode->addNode(triggervertexnode);
  }
  else
  {
    std::cout << "TriggeredVertexSkimmer::CreateNode : " << nodename << " already exists! " << std::endl;
  }
  return Fun4AllReturnCodes::EVENT_OK;
}
