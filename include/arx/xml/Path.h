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
#ifndef ARX_XML_PATH_H
#define ARX_XML_PATH_H

#include <arx/config.h>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/or.hpp>
#include <boost/type_traits/remove_cv.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/proto/proto.hpp>
#include <arx/TypeTraits.h>
#include "NodeWalker.h"
#include "StringProcessor.h"

/* TODO: compile<String>, compile<StringProcessor> - converts internal storage 
 * to string, returns copy of expression. */


namespace arx { namespace xml {
  namespace path_detail {
    namespace proto = boost::proto;
    namespace mpl = boost::mpl;

    /**
     * Metafunction that returns result type of string processor append_element
     * operation, 
     */
    template<class T>
    struct append_element_result {
      typedef void type;
    };

    template<class Result, class Class, class Param1, class Param2>
    struct append_element_result<Result (Class::*)(Param1, Param2) const>:
      remove_cv_reference<Result>
    {};


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
    template<class Node, class NodeWalker, class StringProcessor>
    struct traversal_context: 
      proto::callable_context<const traversal_context<Node, NodeWalker, StringProcessor> >
    {
      traversal_context(const Node &startNode, const NodeWalker &nodeWalker, const StringProcessor &processor): 
        startNode(startNode), nodeWalker(nodeWalker), processor(processor) {}

      typedef Node result_type;

      Node operator()(proto::tag::terminal, const path_start &) const {
        return startNode;
      }

      template<class L, class R>
      Node operator()(proto::tag::divides, const L &l, const R &r) const {
        Node node = proto::eval(l, *this);

        if(nodeWalker.is_null(node))
          return node;

        return nodeWalker.element(node, processor.to_string(proto::eval(r, *this)));
      }

      template<class L, class R>
      Node operator()(proto::tag::multiplies, const L &l, const R &r) const {
        Node node = proto::eval(l, *this);

        if(nodeWalker.is_null(node))
          return node;

        return nodeWalker.attribute(node, processor.to_string(proto::eval(r, *this)));
      }

      const Node &startNode;
      const NodeWalker &nodeWalker;
      const StringProcessor &processor;
    };


    /**
     * Creation context.
     */
    template<class Node, class NodeWalker, class StringProcessor, bool atRoot>
    struct creation_context:
      proto::callable_context<const creation_context<Node, NodeWalker, StringProcessor, atRoot> >
    {
      creation_context(Node &startNode, const NodeWalker &nodeWalker, const StringProcessor &processor): 
        startNode(startNode), nodeWalker(nodeWalker), processor(processor) {}

      typedef Node result_type;

      Node operator()(proto::tag::terminal, const path_start &) const {
        return startNode;
      }

      template<class L, class R>
      Node operator()(proto::tag::divides, const L &l, const R &r) const {
        if(atRoot) {
          /* At root node elements must always be created. */
          creation_context<Node, NodeWalker, StringProcessor, false> ctx(startNode, nodeWalker, processor);
          Node node = proto::eval(l, ctx);
          return nodeWalker.create_element(node, processor.to_string(proto::eval(r, ctx)));
        } else {
          Node node = proto::eval(l, *this);
          auto name = processor.to_string(proto::eval(r, *this));

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

        creation_context<Node, NodeWalker, StringProcessor, false> ctx(startNode, nodeWalker, processor);
        Node node = proto::eval(l, ctx);
        return nodeWalker.create_attribute(node, processor.to_string(proto::eval(r, ctx)));
      }

      Node &startNode;
      const NodeWalker &nodeWalker;
      const StringProcessor &processor;
    };


    /**
     * String conversion context.
     */
    template<class StringProcessor>
    struct string_conversion_context:
      proto::callable_context<const string_conversion_context<StringProcessor> >
    {
      string_conversion_context(const StringProcessor &processor): 
        processor(processor) {}

      typedef decltype(&StringProcessor::append_element) TTT;

      typedef 
        typename append_element_result<
          decltype(&StringProcessor::append_element)
        >::type 
      result_type;

      result_type operator()(proto::tag::terminal, const path_start &) const {
        return result_type();
      }

      template<class T>
      result_type operator()(proto::tag::terminal, const T &pathElement) const {
        return processor.to_string(pathElement);
      }

      template<class L, class R>
      result_type operator()(proto::tag::divides, const L &l, const R &r) const {
        return processor.append_element(proto::eval(l, *this), proto::eval(r, *this));
      }

      template<class L, class R>
      result_type operator()(proto::tag::multiplies, const L &l, const R &r) const {
        return processor.append_attribute(proto::eval(l, *this), proto::eval(r, *this));
      }

      const StringProcessor &processor;
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

      bool is_attribute() const {
        return IS_ATTRIBUTE;
      }

      bool is_element() const {
        return IS_ELEMENT;
      }

      template<class Node, class NodeWalker, class StringProcessor>
      Node traverse(const Node &startNode, const NodeWalker &nodeWalker, const StringProcessor &processor) const {
        traversal_context<Node, NodeWalker, StringProcessor> ctx(startNode, nodeWalker, processor);
        return proto::eval(*this, ctx);
      }

      template<class Node, class NodeWalker>
      Node traverse(const Node &startNode, const NodeWalker &nodeWalker) const {
        typename node_string_processor<Node>::type processor;
        return traverse(startNode, nodeWalker, processor);
      }

      template<class Node>
      Node traverse(const Node &startNode) const {
        typename node_walker<Node>::type nodeWalker;
        return traverse(startNode, nodeWalker);
      }

      template<class Node, class NodeWalker, class StringProcessor>
      Node create(Node &startNode, const NodeWalker &nodeWalker, const StringProcessor &processor) const {
        creation_context<Node, NodeWalker, StringProcessor, true> ctx(startNode, nodeWalker, processor);
        return proto::eval(*this, ctx);
      }

      template<class Node, class NodeWalker>
      Node create(Node &startNode, const NodeWalker &nodeWalker) const {
        typename node_string_processor<Node>::type processor;
        return create(startNode, nodeWalker, processor);
      }

      template<class Node>
      Node create(Node &startNode) const {
        typename node_walker<Node>::type nodeWalker;
        return create(startNode, nodeWalker);
      }

      template<class StringProcessor>
      typename append_element_result<decltype(&StringProcessor::append_element)>::type 
      to_string_process(const StringProcessor &processor) const {
        return proto::eval(*this, string_conversion_context<StringProcessor>(processor));
      }

      template<class String>
      String to_string() const {
        typename string_processor<String>::type processor;
        return to_string_process(processor);
      }

    };


    template<class T>
    struct is_path_expression_impl: 
      mpl::false_ 
    {};

    template<class Expr>
    struct is_path_expression_impl<path_expression<Expr> >: 
      mpl::true_ 
    {};


    /**
     * Metafunction that returns true for path expressions.
     */
    template<class T>
    struct is_path_expression: 
      is_path_expression_impl<typename remove_cv_reference<T>::type>
    {};


    namespace {
      /**
       * Starting terminal for xml path expressions.
       */
      path_expression<proto::terminal<path_start>::type> self = {{{}}};
    }

  } // namespace path_detail


  using path_detail::self;
  using path_detail::is_path_expression;

}} // namespace arx::xml

#endif // ARX_XML_PATH_H
