
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
  SvtxTrackMap *skimmedtracks = findNode::getClass<SvtxTrackMap>(topNode, m_outputtrackNode);
  TrackSeedContainer *skimmedsseeds = findNode::getClass<TrackSeedContainer>(topNode, m_outputsseedNode);
  TrackSeedContainer *skimmedtseeds = findNode::getClass<TrackSeedContainer>(topNode, m_outputtseedNode);
  
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
   m_trackCrossing = track->get_crossing();
   if(m_trackCrossing == 0)
   {
     skimmedtracks->insert(track);

     TrackSeed *sseed = track->get_silicon_seed();
     if(sseed)
     {
       skimmedsseeds->insert(sseed);
     }

     TrackSeed *tseed = track->get_tpc_seed();
     if(tseed)
     {
       skimmedtseeds->insert(tseed);
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
  SvtxVertexMap *skimmedvertex = findNode::getClass<SvtxVertexMap>(topNode, m_outputvertexNode);

  for (const auto &[vertexKey, vertex] : *m_vertexMap)
  {
    if(!vertex) continue; 
    int vertexCrossing = vertex->get_beam_crossing();
    if(vertexCrossing == 0)
    {
      skimmedvertex->insert_clone(vertex);
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
  SvtxTrackMap *test_tracks = findNode::getClass<SvtxTrackMap>(topNode, m_outputtrackNode);
  if (!test_tracks)
  {
    SvtxTrackMap *skimmedtracks = new SvtxTrackMap_v2();
    PHIODataNode<PHObject> *skimmedtracknode;
    if (Verbosity() > 0)
    {
      std::cout << "TriggeredTrackSkimmer::CreateNode : creating " << m_outputtrackNode << std::endl;
    }
    skimmedtracknode = new PHIODataNode<PHObject>(skimmedtracks, m_outputtrackNode, "PHObject");
    svtxnode->addNode(skimmedtracknode);
  }
  else
  {
    std::cout << "TriggeredTrackSkimmer::CreateNode : " << m_outputtrackNode << " already exists! " << std::endl;
  }

  TrackSeedContainer *test_sseeds = findNode::getClass<TrackSeedContainer>(topNode, m_outputsseedNode);
  if (!test_sseeds)
  {
    TrackSeedContainer *skimmedsseeds = new TrackSeedContainer_v1();
    PHIODataNode<PHObject> *skimmedsseednode;
    if (Verbosity() > 0)
    {
      std::cout << "TriggeredTrackSkimmer::CreateNode : creating " << m_outputsseedNode << std::endl;
    }
    skimmedsseednode = new PHIODataNode<PHObject>(skimmedsseeds, m_outputsseedNode, "PHObject");
    svtxnode->addNode(skimmedsseednode);
  }
  else
  {
    std::cout << "TriggeredTrackSkimmer::CreateNode : " << m_outputsseedNode << " already exists! " << std::endl;
  }

  TrackSeedContainer *test_tseeds = findNode::getClass<TrackSeedContainer>(topNode, m_outputtseedNode);
  if (!test_tseeds)
  {
    TrackSeedContainer *skimmedtseeds = new TrackSeedContainer_v1();
    PHIODataNode<PHObject> *skimmedtseednode;
    if (Verbosity() > 0)
    {
      std::cout << "TriggeredTrackSkimmer::CreateNode : creating " << m_outputtseedNode << std::endl;
    }
    skimmedtseednode = new PHIODataNode<PHObject>(skimmedtseeds, m_outputtseedNode, "PHObject");
    svtxnode->addNode(skimmedtseednode);
  }
  else
  {
    std::cout << "TriggeredTrackSkimmer::CreateNode : " << m_outputtseedNode << " already exists! " << std::endl;
  }
  
  SvtxVertexMap *test_vertex = findNode::getClass<SvtxVertexMap>(topNode, m_outputvertexNode);
  if (!test_vertex)
  {
    SvtxVertexMap *skimmedvertex = new SvtxVertexMap_v1();
    PHIODataNode<PHObject> *skimmedvertexnode;
    if (Verbosity() > 0)
    {
      std::cout << "TriggeredVertexSkimmer::CreateNode : creating " << m_outputvertexNode << std::endl;
    }
    skimmedvertexnode = new PHIODataNode<PHObject>(skimmedvertex, m_outputvertexNode, "PHObject");
    svtxnode->addNode(skimmedvertexnode);
  }
  else
  {
    std::cout << "TriggeredVertexSkimmer::CreateNode : " << m_outputvertexNode << " already exists! " << std::endl;
  }
  
  return Fun4AllReturnCodes::EVENT_OK;
  }


