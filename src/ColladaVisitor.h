// System designed by Jeremy D. Cohen, Albert K. Lee, and Mark Bolstad, 2010-2015
// Software designed and implemented by Mark Bolstad, 2010-2015
/* -*-c++-*- */

#ifndef COLLADAVISITOR_H
#define COLLADAVISITOR_H

#include <iostream>
#include <stack>
#include <utility>
#include <vector>

#ifndef Q_MOC_RUN
#include <boost/algorithm/string/iter_find.hpp>
#include <boost/algorithm/string/finder.hpp>
#endif

#include <osg/BoundingBox>
#include <osg/CullFace>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/LightModel>
#include <osg/LightSource>
#include <osg/Node>
#include <osg/Shape>
#include <osg/MatrixTransform>
#include <osg/NodeVisitor>

typedef std::pair< osg::Vec3f, osg::MatrixTransform* > Node_Offset_Pair;

/**
 * @brief An OSG::NodeVisitor applied to Collada formatted input
 * @details ColladaVisitor is an implementation of an osg::NodeVisitor. It's applied to a
 * new scene graph created from Collada input in order to prepare the geometry for
 * consumption by Bullet. Its main purpose is to traverse the
 * geometry and create untransformed representations that result from applying the
 * transform stack onto the geometry.
 */
class ColladaVisitor : public osg::NodeVisitor
{
  public:

    /**
     * @brief Constructor
     * @details Initialize the base class to ensure we traverse all children of the scene graph
     */
    ColladaVisitor()
        : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN )
    {
    }

    /**
     * @brief apply to a Group node
     * @details In the case of a Group node, we do nothing other than continue to the traversal
     * of its children
     *
     * @param node A group node
     */
    virtual void apply( osg::Group& node )
    {
        traverse( node );
    }

    /**
     * @brief apply to a transform
     * @details In the case of a transform, we first prune any nodes without geometry. Second,
     * we flatten the transformation into the geometry and store it based on whether the name
     * indicates it's a visible node.
     *
     * @param node A MatrixTransform
     */
    virtual void apply( osg::MatrixTransform& node )
    {
        std::string const& name = node.getName();
        if ( names.empty() )
            names.push( name );
        else
            names.push( names.top() + "::" + name );
        xform_nodes.push_back( &node );

        traverse( node );

        xform_nodes.pop_back(  );
        names.pop();
#if 0
        if ( node.getChild( 0 )->asGeode( ) != 0)
        {
            if ( node.getChild( 0 )->asGeode()->getNumDrawables() > 0 )
            {
                Node_Offset_Pair flat_node = flatten( node );

                if ( name[0] == '_' && name[ name.size() - 1 ] == '_' )
                    nodes.push_back( flat_node );
                else
                    others.push_back( flat_node );
            }
        }
        else
        {
            osg::LightSource* ls = dynamic_cast<osg::LightSource*>( node.getChild( 0 ) );

            if ( ls )
            {
                lights.push_back( Node_Offset_Pair( osg::Vec3f(), new osg::MatrixTransform( node ) ) );
            }
        }
#endif
    }

    /**
     * @brief apply to geometry
     * @details In the case of geometry, we update the stateset so that we enable
     * two-sided lighting
     *
     * @param node the actual geometry
     */
    void
    apply( osg::Geode &node )
    {
       osg::ref_ptr< osg::LightModel > pLightModel = new osg::LightModel();
       pLightModel->setTwoSided( true );
       osg::StateSet *pState = node.getOrCreateStateSet();
       pState->setAttributeAndModes( pLightModel.get(), osg::StateAttribute::ON );

       Node_Offset_Pair flat_node = flatten( node );
       std::string const &name = names.top();
       std::vector< std::string > strs;
       boost::iter_split(strs, name, boost::algorithm::first_finder("::"));

       bool invisible = false;
       for (std::vector<std::string>::iterator i = strs.begin(); i != strs.end(); ++i)
       {
           if ( (*i)[ 0 ] == '_' && (*i)[ (*i).size() - 1 ] == '_' ) 
            invisible = true;
       }
       if ( invisible )
          nodes.push_back( flat_node );
       else
          others.push_back( flat_node );

       traverse( node );
    }

    void
    apply( osg::LightSource &node )
    {
        osg::MatrixTransform* xform = new osg::MatrixTransform();
        osg::Matrixd mat;
        xform_nodes.front()->computeLocalToWorldMatrix( mat, new osg::NodeVisitor );
        xform->setMatrix( mat );
        xform->addChild( &node );

        lights.push_back( Node_Offset_Pair( osg::Vec3f(), xform ) );
    }

    std::vector< Node_Offset_Pair > others;
    std::vector< Node_Offset_Pair > nodes;
    std::vector< Node_Offset_Pair > lights;

  protected:

    std::stack< std::string > names;
    std::vector< osg::MatrixTransform* > xform_nodes;

    /**
     * @brief flatten the transforms
     * @details Given a transform node, we compute the aggregation of all previous transforms
     * into one matrix. We then extract the geometry, move it's centroid to the origin,
     * and then transform each point and normal. We do this as it simplifies how the geometry
     * is passed to the physics engine as the only transform is a translation back to the
     * centroid.
     *
     * @param motionNode The base transform of the geometry
     * @return The origin-centered transformed geometry and the position of the centroid
     */
    Node_Offset_Pair flatten( osg::Geode& geode )
    {
        osg::MatrixTransform* motionNode = new osg::MatrixTransform();
        for (std::vector<osg::MatrixTransform*>::iterator it = xform_nodes.begin(); it != xform_nodes.end(); ++it)
        {
            motionNode->preMult( (*it)->getMatrix() );
        }

        osg::ref_ptr< osg::MatrixTransform > node;
        node = new osg::MatrixTransform;

        node->setName( names.top() );

        osg::Matrixd mat;
        motionNode->computeLocalToWorldMatrix( mat, new osg::NodeVisitor );

        // In Collada, each MatrixTransform has one Geode node
        osg::Geode* new_geode = new osg::Geode();

        osg::Vec3f center = xform_nodes.front()->computeBound().center();

        // which in turn has one piece of geometry from which
        // we yank out the vertex data
        for ( uint32_t child = 0; child < geode.getNumDrawables(); child++ )
        {
            osg::Geometry* geom = geode.getDrawable( child )->asGeometry();
            osg::Array* vertices = geom->getVertexArray();
            osg::Array* normals = geom->getNormalArray();
            bool has_normals = normals != 0;
            float* v_data = ( float* )( vertices->getDataPointer() );

            osg::Matrixd inv_mat;
            inv_mat.invert_4x3( mat );
            inv_mat.setTrans( 0., 0., 0. );

            float* n_data = 0;
            if ( has_normals )
                n_data = ( float* )( normals->getDataPointer() );

            osg::Vec3Array* new_vertices = new osg::Vec3Array;
            osg::Vec3Array* new_normals = new osg::Vec3Array;
            std::map< int, int > index_mapper;
            int current_index = 0;

            // Bash the geometry to flatten out the transforms and reset
            // so their origin is at 0,0,0
            for ( uint32_t i = 0; i < vertices->getNumElements(); i++ )
            {
                osg::Vec3 v_pt;
                osg::Vec3f v( v_data[3 * i],  v_data[3 * i + 1],  v_data[3 * i + 2] );
                v_pt = ( v * mat ) - center;
                v_data[3 * i] = v_pt.x();
                v_data[3 * i + 1] = v_pt.y();
                v_data[3 * i + 2] = v_pt.z();

                osg::Vec3 n_pt;
                if ( has_normals )
                {
                    osg::Vec3f n( n_data[3 * i],  n_data[3 * i + 1],  n_data[3 * i + 2] );
                    n_pt = inv_mat * n;
                    n_pt.normalize();
                    n_data[3 * i] = n_pt.x();
                    n_data[3 * i + 1] = n_pt.y();
                    n_data[3 * i + 2] = n_pt.z();
                }

                bool found = false;
                uint32_t j;

                // Remove duplicate vertices to make objects
                // truely solid/watertight
                for ( j = 0; !found && j < new_vertices->size(); j++ )
                    found = ( v_pt == ( *new_vertices )[j] );

                if ( found )
                {
                    index_mapper[ i ] = j - 1;
                }
                else
                {
                    new_vertices->push_back( v_pt );
                    if ( has_normals )
                        new_normals->push_back( n_pt );
                    index_mapper[ i ] = current_index++;
                }
            }

            osg::Geometry* geometry = new osg::Geometry();
            new_geode->addDrawable( geometry );
            geometry->setVertexArray( new_vertices );
            if ( has_normals )
            {
                geometry->setNormalArray( new_normals );
                geometry->setNormalBinding( geom->getNormalBinding() );
            }

            // Colors from Collada are stored in the state set
            geometry->setStateSet( geom->getStateSet() );
            if ( geom->getColorArray() != 0 )
                geometry->setColorArray( geom->getColorArray() );
            geometry->setColorBinding( geom->getColorBinding() );

            std::cout << motionNode->getName() << " " << geom->getNumTexCoordArrays() << std::endl;

            for ( uint32_t i = 0; i < geom->getNumTexCoordArrays(); i++ )
            {
                geometry->setTexCoordArray( i, geom->getTexCoordArray( i ) );
            }

            for ( uint32_t i = 0; i < geom->getNumPrimitiveSets(); i++ )
            {
                osg::PrimitiveSet* pSet = geom->getPrimitiveSet( i );
                osg::DrawElementsUInt* side =
                    new osg::DrawElementsUInt( pSet->getMode(), 0 );
                for ( uint32_t j = 0; j < pSet->getNumIndices(); j++ )
                {
                    side->push_back( index_mapper[ pSet->index( j ) ] );
                }

                geometry->addPrimitiveSet( side );
            }
        }

        node->addChild( new_geode );

        return Node_Offset_Pair( center, node.release() );
    }
};

class PrintVisitor : public osg::NodeVisitor
{
  public:

    PrintVisitor()
        : osg::NodeVisitor( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN )
    {}

    void apply( osg::Group& node )
    {
        std::cout << indent << node.getName() << " ( " << node.className() << " )" << std::endl;
        indent.append( " " );
        traverse( node );
        indent.erase( indent.end() - 1 );
    }

    void apply( osg::Node& node )
    {
        std::cout << indent << node.getName() << " ( " << node.className() << " ) " << std::endl;
        traverse( node );
    }

    void apply( osg::Geode& node )
    {
        int count = node.getNumDrawables();

        std::cout << indent << node.getName() << " ( " << node.className() << " ) "
                  << count << std::endl;
        if ( count > 0 )
        {
            indent.append( " " );
            osg::Shape* shape = node.getDrawable( 0 )->getShape();
            if ( shape )
                std::cout << indent << " Shape - " << shape->className() << std::endl;
            osg::Geometry* geo = node.getDrawable( 0 )->asGeometry();
            if ( geo )
            {
                osg::Array* normals = geo->getNormalArray();
                std::cout << indent << " Geometry - " << geo->getVertexArray()->getNumElements();
                if ( normals != 0 )
                    std::cout << ", " << normals->getNumElements();

                std::cout << std::endl;
            }

            indent.erase( indent.end() - 1 );
        }

        traverse( node );
    }

    std::string indent;
};

#endif
