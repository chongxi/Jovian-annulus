// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
/*
 * Modified version of the movie event handler in the osgmovie example. Originally copyright
 * retained below.
 */

/* OpenSceneGraph example, osgmovie.
*
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to deal
*  in the Software without restriction, including without limitation the rights
*  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
*  copies of the Software, and to permit persons to whom the Software is
*  furnished to do so, subject to the following conditions:
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
*  THE SOFTWARE.
*/

#ifndef MOVIE_EVENT_HANDLER
#define MOVIE_EVENT_HANDLER

#include <osg/Geode>
#include <osg/Geometry>
#include <osg/StateSet>
#include <osg/Material>
#include <osg/Texture2D>
#include <osg/TextureRectangle>
#include <osg/CullFace>
#include <osg/ImageStream>
#include <osg/io_utils>

#include <osgGA/EventVisitor>

#include <iostream>

class MovieEventHandler : public osgGA::GUIEventHandler
{
public:

    MovieEventHandler():_trackMouse(false) {}

    void setMouseTracking(bool track) { _trackMouse = track; }
    bool getMouseTracking() const { return _trackMouse; }

    void set(osg::Node* node);

    virtual bool handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv);

    virtual void getUsage(osg::ApplicationUsage& usage) const;

    typedef std::vector< osg::observer_ptr<osg::ImageStream> > ImageStreamList;

protected:

    virtual ~MovieEventHandler() {}

    class FindImageStreamsVisitor : public osg::NodeVisitor
    {
    public:
        FindImageStreamsVisitor(ImageStreamList& imageStreamList):
            osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN ),
            _imageStreamList(imageStreamList) {}

        virtual void apply(osg::Geode& geode)
        {
            apply(geode.getStateSet());

            for(unsigned int i=0;i<geode.getNumDrawables();++i)
            {
                apply(geode.getDrawable(i)->getStateSet());
            }

            traverse(geode);
        }

        virtual void apply(osg::Node& node)
        {
            apply(node.getStateSet());
            traverse(node);
        }

        inline void apply(osg::StateSet* stateset)
        {
            if (!stateset) return;

            osg::StateAttribute* attr = stateset->getTextureAttribute(0,osg::StateAttribute::TEXTURE);
            if (attr)
            {
                osg::Texture2D* texture2D = dynamic_cast<osg::Texture2D*>(attr);
                if (texture2D) apply(dynamic_cast<osg::ImageStream*>(texture2D->getImage()));

                osg::TextureRectangle* textureRec = dynamic_cast<osg::TextureRectangle*>(attr);
                if (textureRec) apply(dynamic_cast<osg::ImageStream*>(textureRec->getImage()));
            }
        }

        inline void apply(osg::ImageStream* imagestream)
        {
            if (imagestream)
            {
                _imageStreamList.push_back(imagestream);
            }
        }

        ImageStreamList& _imageStreamList;

    protected:

        FindImageStreamsVisitor& operator = (const FindImageStreamsVisitor&) { return *this; }

    };


    bool            _trackMouse;
    ImageStreamList _imageStreamList;
    unsigned int    _seekIncr;

};



void MovieEventHandler::set(osg::Node* node)
{
    _imageStreamList.clear();
    if (node)
    {
        FindImageStreamsVisitor fisv(_imageStreamList);
        node->accept(fisv);
    }
}


bool MovieEventHandler::handle(const osgGA::GUIEventAdapter& ea,osgGA::GUIActionAdapter& aa, osg::Object*, osg::NodeVisitor* nv)
{
    switch(ea.getEventType())
    {
        case(osgGA::GUIEventAdapter::MOVE):
        case(osgGA::GUIEventAdapter::PUSH):
        case(osgGA::GUIEventAdapter::RELEASE):
        {
            if (_trackMouse)
            {
                osgViewer::View* view = dynamic_cast<osgViewer::View*>(&aa);
                osgUtil::LineSegmentIntersector::Intersections intersections;
                bool foundIntersection = view==0 ? false :
                    (nv==0 ? view->computeIntersections(ea, intersections) :
                             view->computeIntersections(ea, nv->getNodePath(), intersections));

                if (foundIntersection)
                {

                    // use the nearest intersection
                    const osgUtil::LineSegmentIntersector::Intersection& intersection = *(intersections.begin());
                    osg::Drawable* drawable = intersection.drawable.get();
                    osg::Geometry* geometry = drawable ? drawable->asGeometry() : 0;
                    osg::Vec3Array* vertices = geometry ? dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray()) : 0;
                    if (vertices)
                    {
                        // get the vertex indices.
                        const osgUtil::LineSegmentIntersector::Intersection::IndexList& indices = intersection.indexList;
                        const osgUtil::LineSegmentIntersector::Intersection::RatioList& ratios = intersection.ratioList;

                        if (indices.size()==3 && ratios.size()==3)
                        {
                            unsigned int i1 = indices[0];
                            unsigned int i2 = indices[1];
                            unsigned int i3 = indices[2];

                            float r1 = ratios[0];
                            float r2 = ratios[1];
                            float r3 = ratios[2];

                            osg::Array* texcoords = (geometry->getNumTexCoordArrays()>0) ? geometry->getTexCoordArray(0) : 0;
                            osg::Vec2Array* texcoords_Vec2Array = dynamic_cast<osg::Vec2Array*>(texcoords);
                            if (texcoords_Vec2Array)
                            {
                                // we have tex coord array so now we can compute the final tex coord at the point of intersection.
                                osg::Vec2 tc1 = (*texcoords_Vec2Array)[i1];
                                osg::Vec2 tc2 = (*texcoords_Vec2Array)[i2];
                                osg::Vec2 tc3 = (*texcoords_Vec2Array)[i3];
                                osg::Vec2 tc = tc1*r1 + tc2*r2 + tc3*r3;

                                osg::notify(osg::NOTICE)<<"We hit tex coords "<<tc<<std::endl;

                            }
                        }
                        else
                        {
                            osg::notify(osg::NOTICE)<<"Intersection has insufficient indices to work with";
                        }

                    }
                }
                else
                {
                    osg::notify(osg::NOTICE)<<"No intersection"<<std::endl;
                }
            }
            break;
        }
        case(osgGA::GUIEventAdapter::KEYDOWN):
        {
            if (ea.getKey()=='p')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    osg::ImageStream::StreamStatus playToggle = (*itr)->getStatus();
                    if (playToggle != osg::ImageStream::PLAYING)
                    {
                        std::cout<< (*itr).get() << " Play"<<std::endl;
                        (*itr)->play();
                    }
                    else
                    {
                        // playing, so pause
                        std::cout<< (*itr).get() << " Pause"<<std::endl;
                        (*itr)->pause();
                    }
                }
                return true;
            }
            else if (ea.getKey()=='r')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    std::cout<< (*itr).get() << " Restart"<<std::endl;
                    (*itr)->rewind();
                    (*itr)->play();
                }
                return true;
            }
            else if (ea.getKey()=='>')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    std::cout<<"Seeking"<<std::endl;
                    if(_seekIncr > 3) _seekIncr = 0;
                    double length = (*itr)->getLength();
                    double t_pos = (length/4.0f)*_seekIncr;
                    //(*itr)->rewind();
                    (*itr)->seek(t_pos);
                    (*itr)->play();
                    _seekIncr++;
                }
                return true;
            }
            else if (ea.getKey()=='L')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    if ( (*itr)->getLoopingMode() == osg::ImageStream::LOOPING)
                    {
                        std::cout<< (*itr).get() << " Toggle Looping Off"<<std::endl;
                        (*itr)->setLoopingMode( osg::ImageStream::NO_LOOPING );
                    }
                    else
                    {
                        std::cout<< (*itr).get() << " Toggle Looping On"<<std::endl;
                        (*itr)->setLoopingMode( osg::ImageStream::LOOPING );
                    }
                }
                return true;
            }
            else if (ea.getKey()=='+')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    double tm = (*itr)->getTimeMultiplier();
                    tm += 0.1;
                    (*itr)->setTimeMultiplier(tm);
                    std::cout << (*itr).get() << " Increase speed rate "<< (*itr)->getTimeMultiplier() << std::endl;
                }
                return true;
            }
            else if (ea.getKey()=='-')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    double tm = (*itr)->getTimeMultiplier();
                    tm -= 0.1;
                    (*itr)->setTimeMultiplier(tm);
                    std::cout << (*itr).get() << " Decrease speed rate "<< (*itr)->getTimeMultiplier() << std::endl;
                }
                return true;
            }
            else if (ea.getKey()=='o')
            {
                for(ImageStreamList::iterator itr=_imageStreamList.begin();
                    itr!=_imageStreamList.end();
                    ++itr)
                {
                    std::cout<< (*itr).get() << " Frame rate  "<< (*itr)->getFrameRate() <<std::endl;
                }
                return true;
            }
            return false;
        }

        default:
            return false;
    }
    return false;
}

void MovieEventHandler::getUsage(osg::ApplicationUsage& usage) const
{
    usage.addKeyboardMouseBinding("p","Play/Pause movie");
    usage.addKeyboardMouseBinding("r","Restart movie");
    usage.addKeyboardMouseBinding("l","Toggle looping of movie");
    usage.addKeyboardMouseBinding("+","Increase speed of movie");
    usage.addKeyboardMouseBinding("-","Decrease speed of movie");
    usage.addKeyboardMouseBinding("o","Display frame rate of movie");
    usage.addKeyboardMouseBinding(">","Advance the movie using seek");
}

#endif

