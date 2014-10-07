/*  Copyright (c) BAVC. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license that can
 *  be found in the License.html file in the root of the source tree.
 */

//---------------------------------------------------------------------------
#include "Core/VideoStats.h"
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern "C"
{
#ifndef INT64_C
#define INT64_C(c) (c ## LL)
#define UINT64_C(c) (c ## ULL)
#endif

#include <libavutil/frame.h>
}
#include <QXmlStreamReader>

#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <cfloat>
//---------------------------------------------------------------------------

//***************************************************************************
// Constructor / Destructor
//***************************************************************************

//---------------------------------------------------------------------------
VideoStats::VideoStats (size_t FrameCount, double Duration, size_t FrameCount_Max, double Frequency_)
    :
    Frequency(Frequency_)
{
    // Adaptation for having a graph even with 1 frame
    if (FrameCount<2)
        FrameCount=2;
    if (FrameCount_Max<2)
        FrameCount_Max=2;

    // Status
    IsComplete=false;
    VideoFirstTimeStamp=(uint64_t)-1;

    // Memory management
    Data_Reserved=FrameCount_Max;

    // VideoStats
    memset(Stats_Totals, 0x00, PlotName_Max*sizeof(double));
    memset(Stats_Counts, 0x00, PlotName_Max*sizeof(uint64_t));
    memset(Stats_Counts2, 0x00, PlotName_Max*sizeof(uint64_t));

    // Data - x and y
    x = new double*[4];
    for (size_t j=0; j<4; ++j)
    {
        x[j]=new double[FrameCount_Max];
        memset(x[j], 0x00, FrameCount_Max*sizeof(double));
    }
    y = new double*[PlotName_Max];
    for (size_t j=0; j<PlotName_Max; ++j)
    {
        y[j] = new double[FrameCount_Max];
        memset(y[j], 0x00, FrameCount_Max*sizeof(double));
    }

    // Data - Extra
    durations = new double[FrameCount_Max];
    memset(durations, 0x00, FrameCount_Max*sizeof(double));
    key_frames = new bool[FrameCount_Max];
    memset(key_frames, 0x00, FrameCount_Max*sizeof(bool));

    Data_Reserve(0);

    // Data - Maximums
    x_Current=0;
    x_Current_Max=FrameCount;
    x_Max[0]=x_Current_Max;
    x_Max[1]=Duration;
    x_Max[2]=x_Max[1]/60;
    x_Max[3]=x_Max[2]/60;
    memset(y_Max, 0x00, PlotType_Max*sizeof(double));
}

//---------------------------------------------------------------------------
VideoStats::~VideoStats()
{
    // Data - x and y
    for (size_t j=0; j<4; ++j)
        delete[] x[j];
    delete[] x;

    for (size_t j=0; j<PlotName_Max; ++j)
        delete[] y[j];
    delete[] y;
}

//***************************************************************************
// Status
//***************************************************************************

//---------------------------------------------------------------------------
double VideoStats::State_Get()
{
    if (IsComplete || x_Current_Max==0)
        return 1;

    double Value=((double)x_Current)/x_Current_Max;
    if (Value>=1)
        Value=0.99; // It is not yet complete, so not 100%

    return Value;
}

//***************************************************************************
// External data
//***************************************************************************

//---------------------------------------------------------------------------
void VideoStats::VideoStatsFromExternalData (const string &Data)
{
    // VideoStats from external data
    // XML input
    QXmlStreamReader Xml(Data.c_str());
    while (!Xml.atEnd())
    {
        if (Xml.readNextStartElement())
        {
            if (Xml.name()=="ffprobe")
            {
                while (Xml.readNextStartElement())
                {
                    if (Xml.name()=="frames")
                    {
                        while (Xml.readNextStartElement())
                        {
                            if (Xml.name()=="frame" && Xml.attributes().value("media_type")=="video")
                            {
                                x[0][x_Current]=x_Current;
                                durations[x_Current]=std::atof(Xml.attributes().value("pkt_duration_time").toString().toUtf8().data());
                                string ts=Xml.attributes().value("pkt_pts_time").toString().toUtf8().data();
                                if (ts.empty() || ts=="N/A")
                                    ts=Xml.attributes().value("pkt_dts_time").toString().toUtf8().data(); // Using DTS is PTS is not available
                                if (!ts.empty() && ts!="N/A")
                                {
                                    x[1][x_Current]=std::atof(ts.c_str());
                                    x[2][x_Current]=x[1][x_Current]/60;
                                    x[3][x_Current]=x[2][x_Current]/60;
                                }

                                int Width=atoi(Xml.attributes().value("width").toString().toUtf8().data());
                                int Height=atoi(Xml.attributes().value("height").toString().toUtf8().data());

                                while (Xml.readNextStartElement())
                                {
                                    if (Xml.name()=="tag")
                                    {
                                        PlotName j=PlotName_Max;
                                        for (size_t Plot_Pos=0; Plot_Pos<PlotName_Max; Plot_Pos++)
                                            if (Xml.attributes().value("key")==PerPlotName[Plot_Pos].FFmpeg_Name_2_3)
                                                j=(PlotName)Plot_Pos;

                                        if (j!=PlotName_Max)
                                        {
                                            double value=std::atof(Xml.attributes().value("value").toString().toUtf8().data());
                                            
                                            // Special cases: crop: x2, y2
                                            if (Width && Xml.attributes().value("key")=="lavfi.cropdetect.x2")
                                                y[j][x_Current]=Width-value;
                                            else if (Height && Xml.attributes().value("key")=="lavfi.cropdetect.y2")
                                                y[j][x_Current]=Height-value;
                                            else if (Width && Xml.attributes().value("key")=="lavfi.cropdetect.w")
                                                y[j][x_Current]=Width-value;
                                            else if (Height && Xml.attributes().value("key")=="lavfi.cropdetect.h")
                                                y[j][x_Current]=Height-value;
                                            else
                                                y[j][x_Current]=value;

                                            if (PerPlotName[j].Group1!=PlotType_Max && y_Max[PerPlotName[j].Group1]<y[j][x_Current])
                                                y_Max[PerPlotName[j].Group1]=y[j][x_Current];
                                            if (PerPlotName[j].Group2!=PlotType_Max && y_Max[PerPlotName[j].Group2]<y[j][x_Current])
                                                y_Max[PerPlotName[j].Group2]=y[j][x_Current];

                                            //VideoStats
                                            Stats_Totals[j]+=y[j][x_Current];
                                            if (PerPlotName[j].DefaultLimit!=DBL_MAX)
                                            {
                                                if (y[j][x_Current]>PerPlotName[j].DefaultLimit)
                                                    Stats_Counts[j]++;
                                                if (PerPlotName[j].DefaultLimit2!=DBL_MAX && y[j][x_Current]>PerPlotName[j].DefaultLimit2)
                                                    Stats_Counts2[j]++;
                                            }
                                        }
                                    }
                                    Xml.skipCurrentElement();
                                }

                                QStringRef key_frame_String=Xml.attributes().value("key_frame");
                                if (key_frame_String.size()>0)
                                    key_frames[x_Current]=std::atof(key_frame_String.toString().toUtf8().data());
                                else
                                    key_frames[x_Current]=1; //Forcing key_frame to 1 if it is missing from the XML, for decoding

                                if (x_Max[0]<=x[0][x_Current])
                                {
                                    x_Max[0]=x[0][x_Current];
                                    x_Max[1]=x[1][x_Current];
                                    x_Max[2]=x[2][x_Current];
                                    x_Max[3]=x[3][x_Current];
                                }
                                x_Current++;
                                if (x_Current_Max<=x_Current)
                                {
                                    x_Current_Max=x_Current;
                                    if (x_Current_Max>Data_Reserved)
                                        Data_Reserve(x_Current_Max);
                                }
                            }
                            else
                                Xml.skipCurrentElement();
                        }
                    }
                    else
                        Xml.skipCurrentElement();
                }
            }
            else
                Xml.skipCurrentElement();
        }
    }

    VideoStatsFinish();
}

//---------------------------------------------------------------------------
void VideoStats::VideoStatsFromFrame (struct AVFrame* Frame, int Width, int Height)
{
    AVDictionary * m=av_frame_get_metadata (Frame);
    AVDictionaryEntry* e=NULL;
    string A;
    for (;;)
    {
        e=av_dict_get 	(m, "", e, AV_DICT_IGNORE_SUFFIX);
        if (!e)
            break;
        size_t j=0;
        for (; j<PlotName_Max; j++)
        {
            if (strcmp(e->key, PerPlotName[j].FFmpeg_Name_2_3)==0)
                break;
        }

        if (j<PlotName_Max)
        {
            double value=std::atof(e->value);
                                            
            // Special cases: crop: x2, y2
            if (string(e->key)=="lavfi.cropdetect.x2")
                y[j][x_Current]=Width-value;
            else if (string(e->key)=="lavfi.cropdetect.y2")
                y[j][x_Current]=Height-value;
            else if (string(e->key)=="lavfi.cropdetect.w")
                y[j][x_Current]=Width-value;
            else if (string(e->key)=="lavfi.cropdetect.h")
                y[j][x_Current]=Height-value;
            else
                y[j][x_Current]=value;

            if (PerPlotName[j].Group1!=PlotType_Max && y_Max[PerPlotName[j].Group1]<y[j][x_Current])
                y_Max[PerPlotName[j].Group1]=y[j][x_Current];
            if (PerPlotName[j].Group2!=PlotType_Max && y_Max[PerPlotName[j].Group2]<y[j][x_Current])
                y_Max[PerPlotName[j].Group2]=y[j][x_Current];

            //Stats
            Stats_Totals[j]+=y[j][x_Current];
            if (PerPlotName[j].DefaultLimit!=DBL_MAX)
            {
                if (y[j][x_Current]>PerPlotName[j].DefaultLimit)
                    Stats_Counts[j]++;
                if (PerPlotName[j].DefaultLimit2!=DBL_MAX && y[j][x_Current]>PerPlotName[j].DefaultLimit2)
                    Stats_Counts2[j]++;
            }
        }

        /*
        A+=e->key;
        A+=',';
        A+=e->value;
        A+="\r\n";
        */
    }

    key_frames[x_Current]=Frame->key_frame;

    if (x_Max[0]<=x[0][x_Current])
    {
        x_Max[0]=x[0][x_Current];
        x_Max[1]=x[1][x_Current];
        x_Max[2]=x[2][x_Current];
        x_Max[3]=x[3][x_Current];
    }
    x_Current++;
    if (x_Current_Max<=x_Current)
        x_Current_Max=x_Current;
}

//---------------------------------------------------------------------------
void VideoStats::TimeStampFromFrame (struct AVFrame* Frame, size_t FramePos)
{
    if (Frequency==0)
        return; // Not supported

    if (FramePos>=x_Current_Max)
    {
        x_Current_Max=FramePos+1;
        if (x_Current_Max>Data_Reserved)
            Data_Reserve(x_Current_Max);
    }

    x[0][FramePos]=FramePos;

    int64_t ts=(Frame->pkt_pts==AV_NOPTS_VALUE)?Frame->pkt_dts:Frame->pkt_pts; // Using DTS is PTS is not available // TODO: check if stats are based on DTS or PTS
    //if (ts==AV_NOPTS_VALUE && x_Current)
    //    ts=(int)((x[1][x_Current-1]*x_Current/(x_Current-1))/VideoStream->time_base.num*VideoStream->time_base.den)+VideoFirstTimeStamp; //TODO: understand how to do with first timestamp not being 0 and last timestamp being AV_NOPTS_VALUE e.g. op1a-mpeg2-wave_hd.mxf
    if (VideoFirstTimeStamp==(uint64_t)-1)
        VideoFirstTimeStamp=ts;
    if (ts<VideoFirstTimeStamp)
    {
        for (size_t Pos=0; Pos<x_Current; Pos++)
        {
            x[1][Pos]-=VideoFirstTimeStamp-ts;
            x[2][Pos]=x[1][Pos]/60;
            x[3][Pos]=x[2][Pos]/60;
        }
    }
    ts-=VideoFirstTimeStamp;
    if (ts!=AV_NOPTS_VALUE)
    {
        x[1][FramePos]=((double)ts)/Frequency;
        /*
        if (x[1][x_Current]>VideoDuration)
            VideoDuration=x[1][x_Current];
        */
        x[2][FramePos]=x[1][FramePos]/60;
        x[3][FramePos]=x[2][FramePos]/60;
    }
    if (Frame->pkt_duration!=AV_NOPTS_VALUE)
        durations[FramePos]=((double)Frame->pkt_duration)/Frequency;
}

//---------------------------------------------------------------------------
void VideoStats::VideoStatsFinish ()
{
    // Adaptation
    if (x_Current==1)
    {
        x[0][1]=1;
        x[1][1]=durations[0]?durations[0]:1; //forcing to 1 in case duration is not available
        x[2][1]=durations[0]?(durations[0]/60):1; //forcing to 1 in case duration is not available
        x[3][1]=durations[0]?(durations[0]/3600):1; //forcing to 1 in case duration is not available
        for (size_t Plot_Pos=0; Plot_Pos<PlotName_Max; Plot_Pos++)
            y[Plot_Pos][1]= y[Plot_Pos][0];
        x_Current++;
    }

    // Forcing max values to the last real ones, in case max values were estimated
    if (x_Current)
    {
        x_Max[0]=x[0][x_Current-1];
        x_Max[1]=x[1][x_Current-1];
        x_Max[2]=x[2][x_Current-1];
        x_Max[3]=x[3][x_Current-1];
    }

    x_Current_Max=x_Current;
    IsComplete=true;
}

//---------------------------------------------------------------------------
string VideoStats::StatsToCSV()
{
    stringstream Value;
    Value<<",,,,,,pts,,,,,,,,,,,,,,,YMIN,YLOW,YAVG,YHIGH,YMAX,UMIN,ULOW,UAVG,UHIGH,UMAX,VMIN,VLOW,VAVG,VHIGH,VMAX,YDIF,UDIF,VDIF,SATMIN,SATLOW,SATAVG,SATHIGH,SATMAX,HUEMED,HUEAVG,TOUT,VREP,BRNG,CROPx1,CROPx2,CROPy1,CROPy2,CROPw,CROPh,MSEy,MSEu,MSEv,PSNRy,PSNRu,PSNRv";
    #ifdef _WIN32
        Value<<"\r\n";
    #else
        #ifdef __APPLE__
            Value<<"\r";
        #else
            Value<<"\n";
        #endif
    #endif
    for (size_t Pos=0; Pos<x_Current; Pos++)
    {
        Value<<",,,,,,";
        Value<<fixed<<setprecision(6)<<x[1][Pos];
        Value<<",,,,,,,,,,,,,,";
        for (size_t Pos2=0; Pos2<PlotName_Max; Pos2++)
        {
            Value<<','<<fixed<<setprecision(PerPlotName[Pos2].DigitsAfterComma)<<y[Pos2][Pos];
        }
        #ifdef _WIN32
            Value<<"\r\n";
        #else
            #ifdef __APPLE__
                Value<<"\r";
            #else
                Value<<"\n";
            #endif
        #endif
    }

    return Value.str();
}

//***************************************************************************
// Stats
//***************************************************************************

//---------------------------------------------------------------------------
string VideoStats::Average_Get(PlotName Pos)
{
    if (x_Current==0)
        return string();

    double Value=Stats_Totals[Pos]/x_Current;
    stringstream str;
    str<<fixed<<setprecision(PerPlotName[Pos].DigitsAfterComma)<<Value;
    return str.str();
}

//---------------------------------------------------------------------------
string VideoStats::Average_Get(PlotName Pos, PlotName Pos2)
{
    if (x_Current==0)
        return string();

    double Value=(Stats_Totals[Pos]-Stats_Totals[Pos2])/x_Current;
    stringstream str;
    str<<fixed<<setprecision(PerPlotName[Pos].DigitsAfterComma)<<Value;
    return str.str();
}

//---------------------------------------------------------------------------
string VideoStats::Count_Get(PlotName Pos)
{
    if (x_Current==0)
        return string();

    stringstream str;
    str<<Stats_Counts[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
string VideoStats::Count2_Get(PlotName Pos)
{
    if (x_Current==0)
        return string();

    stringstream str;
    str<<Stats_Counts2[Pos];
    return str.str();
}

//---------------------------------------------------------------------------
string VideoStats::Percent_Get(PlotName Pos)
{
    if (x_Current==0)
        return string();

    double Value=Stats_Counts[Pos]/x_Current;
    stringstream str;
    str<<Value*100<<"%";
    return str.str();
}

//***************************************************************************
// Memory management
//***************************************************************************

//---------------------------------------------------------------------------
void VideoStats::Data_Reserve(size_t NewValue)
{
    // Saving old data
    size_t                      Data_Reserved_Old = Data_Reserved;
    double**                    x_Old = new double*[4];
    memcpy (x_Old, x, sizeof(double*)*4);
    double**                    y_Old = new double*[PlotName_Max];
    memcpy (y_Old, y, sizeof(double*)*PlotName_Max);
    double*                     durations_Old=durations;
    bool*                       key_frames_Old=key_frames;

    // Computing new value
    while (Data_Reserved<NewValue+(1<<18)) //We reserve extra space, minimum 2^18 frames added
        Data_Reserved<<=1;

    // Creating new data - x and y
    x = new double*[4];
    for (size_t j=0; j<4; ++j)
    {
        x[j]=new double[Data_Reserved];
        memset(x[j], 0x00, Data_Reserved*sizeof(double));
        memcpy(x[j], x_Old[j], Data_Reserved_Old*sizeof(double));
    }
    y = new double*[PlotName_Max];
    for (size_t j=0; j<PlotName_Max; ++j)
    {
        y[j] = new double[Data_Reserved];
        memset(y[j], 0x00, Data_Reserved*sizeof(double));
        memcpy(y[j], y_Old[j], Data_Reserved_Old*sizeof(double));
    }

    // Creating new data - Extra
    durations = new double[Data_Reserved];
    memset(durations, 0x00, Data_Reserved*sizeof(double));
    memcpy(durations, durations_Old, Data_Reserved_Old*sizeof(double));
    key_frames = new bool[Data_Reserved];
    memset(key_frames, 0x00, Data_Reserved*sizeof(bool));
    memcpy(key_frames, key_frames_Old, Data_Reserved_Old*sizeof(bool));
}