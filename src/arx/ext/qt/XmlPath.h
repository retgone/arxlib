/* This file is part of ArXLib, a C++ ArX Primitives Library.
 *
 * Copyright (C) 2008-2010 Alexander Fokin <apfokin@gmail.com>
 *
 * ArXLib is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * ArXLib is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License 
 * for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with ArXLib. If not, see <http://www.gnu.org/licenses/>. 
 * 
 * $Id$ */
#ifndef ARX_EXT_QT_XML_PATH_H
#define ARX_EXT_QT_XML_PATH_H

#include "config.h"
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/proto/proto.hpp>
#include "DefaultNodeWalker.h"

namespace arx { namespace xml {
  namespace xml_path_detail {
    namespace proto = boost::proto;
    namespace mpl = boost::mpl;

    /**
     * Tag for the starting element of a path.
     */
    struct path_start {};


    struct path_element_grammar:
      proto::terminal<proto::_>
    {};

    struct path_head_grammar:
      proto::or_<
        proto::terminal<path_start>,
        path_element_grammar,
        proto::divides<path_head_grammar, path_element_grammar>
      >
    {};

    /**
     * Proto grammar for xml path expression.
     */
    struct path_grammar:
      proto::or_<
        proto::terminal<path_start>,
        path_element_grammar,
        proto::divides<path_head_grammar, path_element_grammar>,
        proto::multiplies<path_head_grammar, path_element_grammar>
      >
    {};


    /**
     * Transform that returns true if the given xml path expression defines an
     * attribute access.
     */
    struct path_is_attribute_transform:
      proto::or_<
        proto::when<
          proto::multiplies<proto::_, proto::_>,
          mpl::true_()
        >,
        proto::when<
          proto::terminal<proto::_>,
          mpl::false_()
        >,
        proto::when<
          proto::nary_expr<proto::_, proto::vararg<proto::_> >,
          proto::fold<
            proto::_, 
            mpl::false_(), 
            mpl::or_<path_is_attribute_transform, proto::_state>()
          >
        >
      >
    {};


    template<class Expr>
    struct path_expression;


    /**
     * Proto domain for xml path expressions.
     */
    struct path_domain: 
      proto::domain<proto::pod_generator<path_expression>, path_grammar> 
    {};


    /**
     * Traversal context.
     */
    template<class Node, class NodeWalker>
    struct traversal_context: 
      proto::callable_context<const traversal_context<Node, NodeWalker> >
    {
      traversal_context(const Node &startNode, const NodeWalker &nodeWalker): 
        startNode(startNode), nodeWalker(nodeWalker) {}

      typedef Node result_type;

      Node operator()(proto::tag::terminal, const path_start &) const {
        return startNode;
      }

      template<class L, class R>
      Node operator()(proto::tag::divides, const L &l, const R &r) const {
        Node node = proto::eval(l, *this);

        if(nodeWalker.is_null(node))
          return node;

        return nodeWalker.element(node, proto::eval(r, *this));
      }

      template<class L, class R>
      Node operator()(proto::tag::multiplies, const L &l, const R &r) const {
        Node node = proto::eval(l, *this);

        if(nodeWalker.is_null(node))
          return node;

        return nodeWalker.attribute(node, proto::eval(r, *this));
      }

      const Node &startNode;
      const NodeWalker &nodeWalker;
    };


    /**
     * Creation context.
     */
    template<class Node, class NodeWalker, bool atRoot>
    struct creation_context:
      proto::callable_context<const creation_context<Node, NodeWalker, atRoot> >
    {
      creation_context(Node &startNode, const NodeWalker &nodeWalker): 
        startNode(startNode), nodeWalker(nodeWalker) {}

      typedef Node result_type;

      Node operator()(proto::tag::terminal, const path_start &) const {
        return startNode;
      }

      template<class L, class R>
      Node operator()(proto::tag::divides, const L &l, const R &r) const {
        if(atRoot) {
          /* At root node elements must always be created. */
          creation_context<Node, NodeWalker, false> ctx(startNode, nodeWalker);
          return nodeWalker.create_element(proto::eval(l, ctx), proto::eval(r, ctx));
        } else {
          auto name = proto::eval(r, *this);
          Node node = proto::eval(l, *this);

          /* At non-root nodes elements may be reused. */
          Node element = nodeWalker.element(node, name);
          if(nodeWalker.is_null(element))
            element = nodeWalker.create_element(node, name);
          return element;
        }
      }

      template<class L, class R>
      Node operator()(proto::tag::multiplies, const L &l, const R &r) const {
        static_assert(atRoot, "Attribute access must be a root node of path expression tree.");

        creation_context<Node, NodeWalker, false> ctx(startNode, nodeWalker);
        return nodeWalker.create_attribute(proto::eval(l, ctx), proto::eval(r, ctx));
      }

      Node &startNode;
      const NodeWalker &nodeWalker;
    };


    /**
     * Expression wrapper for property expressions.
     */
    template<class Expr>
    struct path_expression
    {
      typedef path_expression<Expr> this_type;

      BOOST_PROTO_EXTENDS(Expr, this_type, path_domain);

      enum {
        IS_ATTRIBUTE = boost::result_of<path_is_attribute_transform(Expr)>::type::value,
        IS_ELEMENT = !IS_ATTRIBUTE
      };

      bool isAttribute() const {
        return IS_ATTRIBUTE;
      }

      bool isElement() const {
        return IS_ELEMENT;
      }

      template<class Node, class NodeWalker>
      Node traverse(const Node &startNode, const NodeWalker &nodeWalker) const {
        traversal_context<Node, NodeWalker> ctx(startNode, nodeWalker);
        return proto::eval(*this, ctx);
      }

      template<class Node>
      Node traverse(const Node &startNode) const {
        DefaultNodeWalker nodeWalker;
        return traverse(startNode, nodeWalker);
      }

      template<class Node, class NodeWalker>
      Node create(Node &startNode, const NodeWalker &nodeWalker) const {
        creation_context<Node, NodeWalker, true> ctx(startNode, nodeWalker);
        return proto::eval(*this, ctx);
      }

      template<class Node>
      Node create(Node &startNode) const {
        DefaultNodeWalker nodeWalker;
        return create(startNode, nodeWalker);
      }

    };


    /**
     * Starting terminal for xml path expressions.
     */
    path_expression<proto::terminal<path_start>::type> self = {{}};

  } // namespace xml_path_detail


  using xml_path_detail::self;


}} // namespace arx::xml

#endif // ARX_EXT_QT_XML_PATH_H
