/*
  Marc Evans (2018/3/8)
  Final Project Leaf Looper class
*/

#ifndef __LEAF_LOOPER__
#define __LEAF_LOOPER__

#include "Gamma/DFT.h"
#include "allocore/al_Allocore.hpp"
#include "leafOscillators.hpp"
#include "common.hpp"


struct LeafLooper : SoundSource {
  Pose p;
  gam::STFT stft;
  deque<Buffer<Vec3f>> radialStripVertices;
  deque<Buffer<Color>> radialStripColors;
  deque<Mesh> radialStrips;

  Mesh trail;
  deque<Vec3f> trailVertices;
  deque<Color> trailColors;
  int maxTrailLength = 0;
  float trailAlphaDecayFactor = 0.99;
  bool doTrail = true;

  int maxStrips = 60;

  LeafLooperData llData;  // The cuttlebone struct for this lil guy

  float centerFrequency = 4000;
  float centerRadius = 1;

  float amplitudeExpansion = 0.5;

  CombinedLeafOscillator& lfo;
  vector<float> binRadii;
  vector<float> fftMagnitudes;

  Color llColor;

  // DEBUG
  Mesh directionCone;
  bool showDirectionCone = false;

  LeafLooper(CombinedLeafOscillator& _lfo, Color _llColor) 
  : stft(
    FFT_SIZE, FFT_SIZE/4,  // Window size, hop size
    0, gam::HANN, gam::COMPLEX 
  ), lfo(_lfo), llColor(_llColor)
  {
    fftMagnitudes.resize(FFT_SIZE/2);

    trail.primitive(Graphics::TRIANGLE_STRIP);

    // DEBUG
    addCone(directionCone, 0.1, Vec3f(0, 0, -0.8));  // by default we treat objects as facing in the negative z direction
    directionCone.generateNormals();

    setTrailLengthInSeconds(15);
    setBinRadii(1);
  }

  void setBinRadii(float centerRadius, float centerFrequency=4000) {
    binRadii.clear();
    for(int i = 0; i < FFT_SIZE / 2; i++) {
      float freq = float(i) / FFT_SIZE * SAMPLE_RATE;
      binRadii.push_back((log(freq) - log(20)) / (log(centerFrequency) - log(20)) * centerRadius);  
    }
  }

  void setTrailLengthInSeconds(float seconds) {
    maxTrailLength = seconds * 40 * NUM_TRAIL_POINTS_PER_FRAME;
    trailAlphaDecayFactor = exp(log(0.05) * NUM_TRAIL_POINTS_PER_FRAME / maxTrailLength);
  }

  void draw(Graphics& g) {
    g.blendOn();
    g.blendModeTrans();
    g.pushMatrix();

    g.translate(p.pos());
    g.rotate(p);
    for(auto& radialStrip : radialStrips) {
      g.draw(radialStrip);
    }
    if(showDirectionCone) {
      g.draw(directionCone);
    }
    g.popMatrix();
    if(doTrail) {
      g.draw(trail);
    }
  }

  void operator()(float s) {
    if(stft(s)) {
      // Loop through all the bins
      for(unsigned k=0; k<stft.numBins(); ++k){
        // higher pow reduces visual decay time
        // multiplier gets it to roughly the right level
        // tanh squashes it between 0 and 1
        fftMagnitudes[k] = tanh(pow(stft.bin(k).mag(), 1.3) * 1000.0);
      }
    }
  }

  void pushNewStrip(float phase, float phase2) {
    // ADD A NEW STRIP OF VERTICES AND COLORS
    Buffer<Vec3f> newRadialStripVertices;
    Buffer<Color> newRadialStripColors;

    for(int i = 0; i < FFT_SIZE / 2; i++) {
      float radius = binRadii.at(i) * lfo.getRadius(phase);
      float angle = lfo.getAngle(phase) + rnd::uniformS() * amplitudeExpansion * fftMagnitudes.at(i);
      float angle2 = lfo.getAngle(phase2) + rnd::uniformS() * amplitudeExpansion * fftMagnitudes.at(i);
      // // loud bins get visualized with random displacement of the second angle
      // float adjustedAngle2 = angle2 + rnd::uniformS() * amplitudeExpansion * fftMagnitudes.at(i); // 0.5*M_PI; 
      // add randomness to phase 2, and clip it
      newRadialStripVertices.append(Vec3f(cos(angle2)*radius, cos(angle)*sin(angle2)*radius, -sin(angle)*radius));
      newRadialStripColors.append(Color(llColor.r, llColor.g, llColor.b, fftMagnitudes.at(i)));
    }
    radialStripVertices.push_back(newRadialStripVertices);
    radialStripColors.push_back(newRadialStripColors);
    pushNewStripMesh();

    std::vector<int> binsByMagnitude(fftMagnitudes.size());
    std::iota(binsByMagnitude.begin(), binsByMagnitude.end(), 0);
    // std::vector<float>& fftMagnitudes = this->fftMagnitudes;
    std::sort(binsByMagnitude.begin(), binsByMagnitude.end(),
      [&](int a, int b) {  
        return (fftMagnitudes.at(a) > fftMagnitudes.at(b));  
      }
    );
    std::vector<Vec3f> topMagVertices;
    std::vector<Color> topMagColors;
    for(int i = 0; i < NUM_TRAIL_POINTS_PER_FRAME; ++i) {
      int thisBin = binsByMagnitude.at(i);
      Vec3f& thisVertex = newRadialStripVertices[thisBin];
      // need to translate the vertex to world coordinates. This was a little tricky...
      topMagVertices.push_back(p.pos() + p.ur() * thisVertex.x + p.uu() * thisVertex.y - p.uf() * thisVertex.z);
      Color thisColor(newRadialStripColors[thisBin]);
      thisColor.a *= 0.2;
      topMagColors.push_back(thisColor);
    }
    if(doTrail) {
      pushNewTrailPoints(topMagVertices, topMagColors);
    }
  }

  private:
  void pushNewTrailPoints(std::vector<Vec3f>& newVertices, std::vector<Color>& newColors) {
    llData.shiftTrail();
    PseudoMesh<NUM_TRAIL_POINTS_PER_FRAME>& newestTrailPoints = llData.latestTrailPoints[REDUNDANCY-1];

    for(int i=0; i < NUM_TRAIL_POINTS_PER_FRAME; ++i) {
      trailVertices.push_back(newVertices.at(i));
      newestTrailPoints.vertices[i] = newVertices.at(i);
      trailColors.push_back(newColors.at(i));
      newestTrailPoints.colors[i] = newColors.at(i);
    }

    while(trailVertices.size() > maxTrailLength) {
      trailVertices.pop_front();
      trailColors.pop_front();
    }

    for (Color& c : trailColors) { c.a *= trailAlphaDecayFactor; }

    trail.vertices().reset();
    trail.colors().reset();

    for(int i = 0; i < trailVertices.size() - NUM_TRAIL_POINTS_PER_FRAME; ++i) { 
      trail.vertex(trailVertices.at(i));
      trail.color(trailColors.at(i));
      trail.vertex(trailVertices.at(i + NUM_TRAIL_POINTS_PER_FRAME)); 
      trail.color(trailColors.at(i + NUM_TRAIL_POINTS_PER_FRAME));
    }
  }

  void pushNewStripMesh() {
    // Construct a new strip mesh out of the last two sets of vertices and colors we added
    if(radialStripVertices.size() > 1) {
      Buffer<Vec3f>& newRadialStripVertices = radialStripVertices.at(radialStripVertices.size()-1);
      Buffer<Color>& newRadialStripColors = radialStripColors.at(radialStripColors.size()-1);
      Buffer<Vec3f>& lastRadialStripVertices = radialStripVertices.at(radialStripVertices.size()-2);
      Buffer<Color>& lastRadialStripColors = radialStripColors.at(radialStripColors.size()-2);
      Mesh radialStrip;

      llData.shiftStrips();
      PseudoMesh<FFT_SIZE>& newestPseudoStrip = llData.latestStrips[REDUNDANCY-1];

      radialStrip.primitive(Graphics::TRIANGLE_STRIP);
      for(int i = 0; i < FFT_SIZE / 2; i++) {
        radialStrip.vertex(lastRadialStripVertices[i]);
        newestPseudoStrip.vertices[2*i] = lastRadialStripVertices[i];
        Color lc = lastRadialStripColors[i];
        lc.a *= VISUAL_DECAY;
        radialStrip.color(lc);
        newestPseudoStrip.colors[2*i] = lastRadialStripColors[i];
        radialStrip.vertex(newRadialStripVertices[i]);
        newestPseudoStrip.vertices[2*i + 1] = newRadialStripVertices[i];
        radialStrip.color(newRadialStripColors[i]);
        newestPseudoStrip.colors[2*i + 1] = newRadialStripColors[i];
      }
      for(int i = 0; i < radialStrips.size(); ++i) {
        for(auto& color : radialStrips.at(i).colors()) {
          color.a *= VISUAL_DECAY;
        }
      }
      radialStrips.push_back(radialStrip);
    }
    if(radialStrips.size() > maxStrips) {
      radialStrips.pop_front();
      radialStripVertices.pop_front();
    }
  }
};

#endif