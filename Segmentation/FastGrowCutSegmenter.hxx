
#ifndef FAST_GrowCutSegmenter_HXX
#define FAST_GrowCutSegmenter_HXX


#include "FastGrowCutSegmenter.h"

#include "itkImageRegionIterator.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkNumericTraits.h"
#include "itkRegionOfInterestImageFilter.h"


namespace FGC {

inline void FastGrowCutHeapNode::Print() {
    FibHeapNode::Print();
//    mexPrintf( "%f (%d)" , N , IndexV );
}

inline void FastGrowCutHeapNode::operator =(double NewKeyVal) {
    FastGrowCutHeapNode Tmp;
    Tmp.N = N = NewKeyVal;
    FHN_Assign(Tmp);
}

inline void FastGrowCutHeapNode::operator =(FibHeapNode& RHS) {
    FHN_Assign(RHS);
    N = ((FastGrowCutHeapNode&) RHS).N;
}

inline int  FastGrowCutHeapNode::operator ==(FibHeapNode& RHS) {
    if (FHN_Cmp(RHS)) return 0;
    return N == ((FastGrowCutHeapNode&) RHS).N ? 1 : 0;
}

inline int  FastGrowCutHeapNode::operator <(FibHeapNode& RHS) {
    int X;
    if ((X=FHN_Cmp(RHS)) != 0)
        return X < 0 ? 1 : 0;
    return N < ((FastGrowCutHeapNode&) RHS).N ? 1 : 0;
}

template<typename SrcPixelType, typename LabPixelType>
FastGrowCut<SrcPixelType, LabPixelType>
::FastGrowCut() {
    m_heap = NULL;
    m_hpNodes = NULL;
    m_bSegInitialized = false;

}

template<typename SrcPixelType, typename LabPixelType>
FastGrowCut<SrcPixelType, LabPixelType>
::~FastGrowCut() {
    if(m_heap != NULL) {
        delete m_heap;
        m_heap = NULL;
    }
    if(m_hpNodes != NULL) {
        delete []m_hpNodes;
        m_hpNodes = NULL;
    }

    std::cout << "FastGrowCut destroyed\n";
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::SetSourceImage(const std::vector<SrcPixelType>& imSrc) {

    m_imSrc = imSrc;
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::SetSeedlImage(std::vector<LabPixelType>& imSeed) {

    m_imSeed = imSeed;
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::SetWorkMode(bool bSegUnInitialized ) {

    m_bSegInitialized = bSegUnInitialized;
}
template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::SetImageSize(const std::vector<long>& imSize) {

    m_imSize = imSize;
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::InitializationAHP() {

     m_DIMX = m_imSize[0];
     m_DIMY = m_imSize[1];
     m_DIMZ = m_imSize[2];
     m_DIMXY = m_DIMX*m_DIMY;
     m_DIMXYZ = m_DIMXY*m_DIMZ;

    if((m_heap = new FibHeap) == NULL || (m_hpNodes = new FastGrowCutHeapNode[m_DIMXYZ+1]) == NULL) {
        std::cerr << "Memory allocation failed-- ABORTING.\n";
        raise(SIGABRT);
    }
    m_heap->ClearHeapOwnership();

    long  i,j,k, index;
     if(!m_bSegInitialized) {
         m_imLab.resize(m_DIMXYZ);
         m_imDist.resize(m_DIMXYZ);
         m_imLabPre.resize(m_DIMXYZ);
         m_imDistPre.resize(m_DIMXYZ);
         // Compute index offset
         m_indOff.clear();
         long ix,iy,iz;
         for(ix = -1; ix <= 1; ix++)
             for(iy = -1; iy <= 1; iy++)
                 for(iz = -1; iz <= 1; iz++) {
                     if(!(ix == 0 && iy == 0 && iz == 0)) {
                         m_indOff.push_back(ix + iy*m_DIMX + iz*m_DIMXY);
                     }
                 }

         // Determine neighborhood size at each vertice
         m_NBSIZE = std::vector<unsigned char>(m_DIMXYZ, 0);
         for(i = 1; i < m_DIMX - 1; i++)
             for(j = 1; j < m_DIMY - 1; j++)
                 for(k = 1; k < m_DIMZ - 1; k++) {
                     index = i + j*m_DIMX + k*m_DIMXY;
                     m_NBSIZE[index] = NNGBH;
                 }

         for(index = 0; index < m_DIMXYZ; index++) {
             m_imLab[index] = m_imSeed[index];
             if(m_imLab[index] == 0) {
                 m_hpNodes[index] = (float)DIST_INF;
                 m_imDist[index] = DIST_INF;
             }
             else {
                 m_hpNodes[index] = (float)DIST_EPSION;
                 m_imDist[index] = DIST_EPSION;
              }

              m_heap->Insert(&m_hpNodes[index]);
              m_hpNodes[index].SetIndexValue(index);
         }
     }
     else {
         for(index = 0; index < m_DIMXYZ; index++) {
             if(m_imSeed[index] != 0 && m_imSeed[index] != m_imLabPre[index]) {
//             if(m_imSeed[index] != 0 && (m_imDistPre[index] != 0 ||  (m_imDistPre[index] == 0 && m_imSeed[index] != m_imLabPre[index]))) {
                 m_hpNodes[index] = (float)DIST_EPSION;
                 m_imDist[index] = DIST_EPSION;
                 m_imLab[index] = m_imSeed[index];
             }
             else {
                 m_hpNodes[index] = (float)DIST_INF;
                 m_imDist[index] = DIST_INF;
                 m_imLab[index] = 0;
             }
             m_heap->Insert(&m_hpNodes[index]);
             m_hpNodes[index].SetIndexValue(index);
         }
     }
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::DijkstraBasedClassificationAHP() {

    FastGrowCutHeapNode *hnMin, hnTmp;
    float t, tOri, tSrc;
    long i, index, indexNgbh;
    LabPixelType labSrc;
    SrcPixelType pixCenter;

    // Insert 0 then extract it, which will balance heap
    m_heap->Insert(&hnTmp); m_heap->ExtractMin();

    long k = 0;

    // Adpative Dijkstra
    if(m_bSegInitialized) {
        while(!m_heap->IsEmpty()) {
            hnMin = (FastGrowCutHeapNode *) m_heap->ExtractMin();
            index = hnMin->GetIndexValue();
            tSrc = hnMin->GetKeyValue();

            // stop propagation when the new distance is larger than the previous one
            if(tSrc == DIST_INF) break;
            if(tSrc > m_imDistPre[index]) {
                m_imDist[index] = m_imDistPre[index];
                m_imLab[index] = m_imLabPre[index];
                continue;
            }

            labSrc = m_imLab[index];
            m_imDist[index] = tSrc;

             // Update neighbors
            pixCenter = m_imSrc[index];
            for(i = 0; i < m_NBSIZE[index]; i++) {

                    indexNgbh = index + m_indOff[i];
                    tOri = m_imDist[indexNgbh];
                    t = std::abs(pixCenter - m_imSrc[indexNgbh]) + tSrc;
                    if(tOri > t) {
                        m_imDist[indexNgbh] = t;
                        m_imLab[indexNgbh] = labSrc;

                        hnTmp = m_hpNodes[indexNgbh];
                        hnTmp.SetKeyValue(t);
                        m_heap->DecreaseKey(&m_hpNodes[indexNgbh], hnTmp);
                    }

            }

            k++;

        }

        // Update previous labels and distance information
        for(long i = 0; i < m_DIMXYZ; i++) {
            if(m_imDist[i] < DIST_INF) {
                m_imLabPre[i] = m_imLab[i];
                m_imDistPre[i] = m_imDist[i];
            }
        }
        std::copy(m_imLabPre.begin(), m_imLabPre.end(), m_imLab.begin());
        std::copy(m_imDistPre.begin(), m_imDistPre.end(), m_imDist.begin());

//        m_imLab = m_imLabPre;
//        m_imDist = m_imDistPre;
    }
     // Normal Dijkstra to be used in initializing the segmenter for the current image
    else {
        while(!m_heap->IsEmpty()) {
            hnMin = (FastGrowCutHeapNode *) m_heap->ExtractMin();
            index = hnMin->GetIndexValue();
            tSrc = hnMin->GetKeyValue();
            labSrc = m_imLab[index];
            m_imDist[index] = tSrc;

             // Update neighbors
            pixCenter = m_imSrc[index];
            for(i = 0; i < m_NBSIZE[index]; i++) {

                    indexNgbh = index + m_indOff[i];
                    tOri = m_imDist[indexNgbh];
                    t = std::abs(pixCenter - m_imSrc[indexNgbh]) + tSrc;
                    if(tOri > t) {
                        m_imDist[indexNgbh] = t;
                        m_imLab[indexNgbh] = labSrc;

                        hnTmp = m_hpNodes[indexNgbh];
                        hnTmp.SetKeyValue(t);
                        m_heap->DecreaseKey(&m_hpNodes[indexNgbh], hnTmp);
                    }

            }

            k++;

        }
    }

    std::copy(m_imLab.begin(), m_imLab.end(), m_imLabPre.begin());
    std::copy(m_imDist.begin(), m_imDist.end(), m_imDistPre.begin());

    // Release memory
    if(m_heap != NULL) {
        delete m_heap;
        m_heap = NULL;
    }
    if(m_hpNodes != NULL) {
        delete []m_hpNodes;
        m_hpNodes = NULL;
    }
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::DoSegmentation() {

    InitializationAHP();

    DijkstraBasedClassificationAHP();
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::GetLabeImage(std::vector<LabPixelType>& imLab) {

    imLab.resize(m_DIMXYZ);
    std::copy(m_imLab.begin(), m_imLab.end(), imLab.begin());
}

template<typename SrcPixelType, typename LabPixelType>
void FastGrowCut<SrcPixelType, LabPixelType>
::GetForegroundmage(std::vector<LabPixelType>& imFgrd) {

    long index;
    imFgrd.resize(m_DIMXYZ);
    index = 0;
    for(index = 0; index < m_DIMXYZ; index++) {
        imFgrd[index] = m_imLab[index] == 1 ? 1 :0;
    }

}
} // end FGC
#endif
