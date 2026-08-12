// Tell emacs that this is a C++ source
//  -*- C++ -*-.
#ifndef TRIGGEREDTRACKSKIMMER_H
#define TRIGGEREDTRACKSKIMMER_H

#include <fun4all/SubsysReco.h>

#include <string>

class PHCompositeNode;
class SvtxTrack;
class SvtxTrackMap;
class SvtxVertex;
class SvtxVertexMap;
class TrackSeedContainer;

class TriggeredTrackSkimmer : public SubsysReco
{
 public:

  TriggeredTrackSkimmer(const std::string &name = "TriggeredTrackSkimmer");

  ~TriggeredTrackSkimmer() override;

  /** Called during initialization.
      Typically this is where you can book histograms, and e.g.
      register them to Fun4AllServer (so they can be output to file
      using Fun4AllServer::dumpHistos() method).
   */
  int Init(PHCompositeNode *topNode) override;

  /** Called for first event when run number is known.
      Typically this is where you may want to fetch data from
      database, because you know the run number. A place
      to book histograms which have to know the run number.
   */
  int InitRun(PHCompositeNode *topNode) override;

  /** Called for each event.
      This is where you do the real work.
   */
  int process_event(PHCompositeNode *topNode) override;

  /// Clean up internals after each event.
  int ResetEvent(PHCompositeNode *topNode) override;

  /// Called at the end of each run.
  int EndRun(const int runnumber) override;

  /// Called at the end of all processing.
  int End(PHCompositeNode *topNode) override;

  /// Reset
  int Reset(PHCompositeNode * /*topNode*/) override;

  void Print(const std::string &what = "ALL") const override;

  int CreateNode(PHCompositeNode *topNode);
  int CreateTrackNode(PHCompositeNode *topNode, PHCompositeNode *svtxNode, std::string nodename);
  int CreateSeedNode(PHCompositeNode *topNode, PHCompositeNode *svtxNode, std::string nodename);
  int CreateVertexNode(PHCompositeNode *topNode, PHCompositeNode *svtxNode, std::string nodename);
  
 private:
  SvtxTrackMap *m_trackMap = nullptr;
  std::string m_trackMapName = "SvtxTrackMap";
  std::string m_triggertrackNode = "TriggerSvtxTrackMap";
  std::string m_streamtrackNode = "StreamSvtxTrackMap";

  TrackSeedContainer *m_siliconSeedMap = nullptr;
  std::string m_siliconSeedMapName = "SiliconTrackSeedContainer";
  std::string m_triggersseedNode = "TriggerSiliconTrackSeedContainer";
  std::string m_streamsseedNode = "StreamSiliconTrackSeedContainer";
  
  TrackSeedContainer *m_TPCSeedMap = nullptr;
  std::string m_TPCSeedMapName = "TpcTrackSeedContainer";
  std::string m_triggertseedNode = "TriggerTpcTrackSeedContainer";
  std::string m_streamtseedNode = "StreamTpcTrackSeedContainer";
  
  TrackSeedContainer *m_svtxSeedMap = nullptr;
  std::string m_svtxSeedMapName = "SvtxTrackSeedContainer";
  std::string m_triggerseedNode = "TriggerSvtxTrackSeedContainer";
  std::string m_streamseedNode = "StreamSvtxTrackSeedContainer";
  
  SvtxVertexMap *m_vertexMap = nullptr;
  std::string m_vertexMapName = "SvtxVertexMap";
  std::string m_triggervertexNode = "TriggerSvtxVertexMap";
  std::string m_streamvertexNode = "StreamSvtxVertexMap";

};

#endif // TRIGGEREDTRACKSKIMMER_H
