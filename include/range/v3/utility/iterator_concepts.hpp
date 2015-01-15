/// \file
// Range v3 library
//
//  Copyright Eric Niebler 2013-2014
//
//  Use, modification and distribution is subject to the
//  Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)
//
// Project home: https://github.com/ericniebler/range-v3
//

#ifndef RANGES_V3_UTILITY_ITERATOR_CONCEPTS_HPP
#define RANGES_V3_UTILITY_ITERATOR_CONCEPTS_HPP

#include <iterator>
#include <type_traits>
#include <range/v3/utility/meta.hpp>
#include <range/v3/utility/move.hpp>
#include <range/v3/utility/swap.hpp>
#include <range/v3/utility/concepts.hpp>
#include <range/v3/utility/invokable.hpp>
#include <range/v3/utility/functional.hpp>

namespace ranges
{
    inline namespace v3
    {
        /// \addtogroup group-core
        /// @{
        struct weak_input_iterator_tag
        {};

        struct input_iterator_tag
          : weak_input_iterator_tag
        {};

        struct forward_iterator_tag
          : input_iterator_tag
        {};

        struct bidirectional_iterator_tag
          : forward_iterator_tag
        {};

        struct random_access_iterator_tag
          : bidirectional_iterator_tag
        {};
        /// @}

        /// \cond
        namespace detail
        {
            ////////////////////////////////////////////////////////////////////////////////////////
            template<typename T>
            struct as_ranges_iterator_category
            {
                using type = T;
            };

            template<>
            struct as_ranges_iterator_category<std::input_iterator_tag>
            {
                using type = ranges::input_iterator_tag;
            };

            template<>
            struct as_ranges_iterator_category<std::forward_iterator_tag>
            {
                using type = ranges::forward_iterator_tag;
            };

            template<>
            struct as_ranges_iterator_category<std::bidirectional_iterator_tag>
            {
                using type = ranges::bidirectional_iterator_tag;
            };

            template<>
            struct as_ranges_iterator_category<std::random_access_iterator_tag>
            {
                using type = ranges::random_access_iterator_tag;
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            template<typename Tag, typename Reference>
            struct as_std_iterator_category;

            template<typename Reference>
            struct as_std_iterator_category<ranges::weak_input_iterator_tag, Reference>
            {
                // Not a valid C++14 iterator
            };

            template<typename Reference>
            struct as_std_iterator_category<ranges::input_iterator_tag, Reference>
            {
                using type = std::input_iterator_tag;
            };

            template<typename Reference>
            struct as_std_iterator_category<ranges::forward_iterator_tag, Reference>
            {
                using type = meta::if_<
                    std::is_reference<Reference>,
                    std::forward_iterator_tag,
                    std::input_iterator_tag>;
            };

            template<typename Reference>
            struct as_std_iterator_category<ranges::bidirectional_iterator_tag, Reference>
            {
                using type = meta::if_<
                    std::is_reference<Reference>,
                    std::bidirectional_iterator_tag,
                    std::input_iterator_tag>;
            };

            template<typename Reference>
            struct as_std_iterator_category<ranges::random_access_iterator_tag, Reference>
            {
                using type = meta::if_<
                    std::is_reference<Reference>,
                    std::random_access_iterator_tag,
                    std::input_iterator_tag>;
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            template<typename T, typename Enable = void>
            struct pointer_type
            {};

            template<typename T>
            struct pointer_type<T *, void>
            {
                using type = T *;
            };

            template<typename T>
            struct pointer_type<T, enable_if_t<std::is_class<T>::value, void>>
            {
                using type = decltype(std::declval<T>().operator->());
            };

            ////////////////////////////////////////////////////////////////////////////////////////
            template<typename T, typename Enable = void>
            struct iterator_category_type
            {};

            template<typename T>
            struct iterator_category_type<T *, void>
            {
                using type = ranges::random_access_iterator_tag;
            };

            template<typename T>
            struct iterator_category_type<T, void_t<typename T::iterator_category>>
              : as_ranges_iterator_category<typename T::iterator_category>
            {};
        }
        /// \endcond

        /// \addtogroup group-concepts
        /// @{
        template<typename T>
        struct pointer_type
          : detail::pointer_type<uncvref_t<T>>
        {};

        template<typename T>
        struct iterator_category_type
          : detail::iterator_category_type<uncvref_t<T>>
        {};

        namespace concepts
        {
            struct Readable
              : refines<SemiRegular>
            {
                // Associated types
                template<typename I>
                using value_t = meta::eval<value_type<I>>;

                template<typename I>
                using reference_t = decltype(*std::declval<I>());

                template<typename I>
                using rvalue_reference_t = decltype(indirect_move(std::declval<I>()));

                template<typename I>
                using common_reference_t =
                    ranges::common_reference_t<reference_t<I> &&, value_t<I> &>;

                template<typename I>
                using pointer_t = meta::eval<pointer_type<I>>;

                template<typename I>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        // The value, reference and rvalue reference types are related
                        // through the CommonReference concept.
                        concepts::model_of<CommonReference, reference_t<I> &&, value_t<I> &>(),
                        concepts::model_of<CommonReference, reference_t<I> &&, rvalue_reference_t<I> &&>(),
                        concepts::model_of<CommonReference, rvalue_reference_t<I> &&, value_t<I> const &>(),
                        // This ensures that there is a way to move from reference type
                        // to the rvalue reference type via the 2-argument version of indirect_move:
                        concepts::same_type(indirect_move(i), indirect_move(i, *i))
                        // Axiom: indirect_move(i) and indirect_move(i, *i) are required to be
                        // semantically equivalent. There is no requirement that indirect_move(i)
                        // be implemented in terms of indirect_move(i, *i). indirect_move(i, *i)
                        // should not read from i.
                    ));
            };

            struct MoveWritable
              : refines<SemiRegular(_1)>
            {
                template<typename Out, typename T>
                auto requires_(Out o, T) -> decltype(
                    concepts::valid_expr(
                        *o = std::move(val<T>())
                    ));
            };

            struct Writable
              : refines<MoveWritable>
            {
                template<typename Out, typename T>
                auto requires_(Out o, T) -> decltype(
                    concepts::valid_expr(
                        *o = val<T>()
                    ));
            };

            struct IndirectlyMovable
            {
                template<typename I, typename O, typename P = ident>
                auto requires_(I i, O o, P = P{}) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Readable, I>(),
                        concepts::model_of<SemiRegular, O>(),
                        concepts::model_of<Convertible, Readable::rvalue_reference_t<I> &&,
                            Readable::value_t<I>>(),
                        concepts::model_of<RegularInvokable, P, Readable::rvalue_reference_t<I> &&>(),
                        concepts::model_of<RegularInvokable, P, Readable::value_t<I> &&>(),
                        concepts::model_of<MoveWritable, O,
                            Invokable::result_t<P, Readable::rvalue_reference_t<I> &&>>(),
                        concepts::model_of<MoveWritable, O,
                            Invokable::result_t<P, Readable::value_t<I> &&>>()
                    ));
            };

            struct IndirectlyCopyable
              : refines<IndirectlyMovable>
            {
                template<typename I, typename O, typename P = ident>
                auto requires_(I i, O o, P = P{}) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Readable, I>(),
                        concepts::model_of<SemiRegular, O>(),
                        concepts::model_of<Convertible, Readable::reference_t<I>,
                            Readable::value_t<I>>(),
                        concepts::model_of<RegularInvokable, P, Readable::reference_t<I> &&>(),
                        concepts::model_of<RegularInvokable, P, Readable::common_reference_t<I> &&>(),
                        concepts::model_of<RegularInvokable, P, Readable::value_t<I> const &>(),
                        concepts::model_of<Writable, O,
                            Invokable::result_t<P, Readable::reference_t<I> &&>>(),
                        concepts::model_of<Writable, O,
                            Invokable::result_t<P, Readable::common_reference_t<I> &&>>(),
                        concepts::model_of<Writable, O,
                            Invokable::result_t<P, Readable::value_t<I> const &>>()
                    ));
            };

            struct IndirectlySwappable
            {
                template<typename I1, typename I2>
                auto requires_(I1 i1, I2 i2) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Readable, I1>(),
                        concepts::model_of<Readable, I2>(),
                        (ranges::indirect_swap(i1, i2), 42),
                        (ranges::indirect_swap(i1, i1), 42),
                        (ranges::indirect_swap(i2, i2), 42),
                        (ranges::indirect_swap(i2, i1), 42)
                    ));
            };

            struct WeaklyIncrementable
              : refines<SemiRegular>
            {
                // Associated types
                template<typename I>
                using difference_t = meta::eval<difference_type<I>>;

                template<typename I>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        concepts::is_true(std::is_integral<difference_t<I>>{}),
                        concepts::has_type<I &>(++i),
                        ((i++), 42)
                    ));
            };

            struct Incrementable
              : refines<Regular, WeaklyIncrementable>
            {
                template<typename I>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        concepts::has_type<I>(i++)
                    ));
            };

            struct WeakIterator
              : refines<WeaklyIncrementable, Copyable>
            {
                template<typename I>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        *i
                    ));
            };

            struct Iterator
              : refines<WeakIterator, EqualityComparable>
            {};

            struct WeakOutputIterator
              : refines<WeakIterator(_1), Writable>
            {};

            struct OutputIterator
              : refines<WeakOutputIterator, Iterator(_1)>
            {};

            struct WeakInputIterator
              : refines<WeakIterator, Readable>
            {
                // Associated types
                // value_t from readable
                // distance_t from WeaklyIncrementable
                template<typename I>
                using category_t = meta::eval<ranges::iterator_category_type<I>>;

                template<typename I>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Derived, category_t<I>, ranges::weak_input_iterator_tag>(),
                        concepts::model_of<Readable>(i++)
                    ));
            };

            struct InputIterator
              : refines<WeakInputIterator, Iterator, EqualityComparable>
            {
                template<typename I>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Derived, category_t<I>, ranges::input_iterator_tag>()
                    ));
            };

            struct ForwardIterator
              : refines<InputIterator, Incrementable>
            {
                template<typename I>
                auto requires_(I) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Derived, category_t<I>, ranges::forward_iterator_tag>()
                    ));
            };

            struct BidirectionalIterator
              : refines<ForwardIterator>
            {
                template<typename I>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Derived, category_t<I>, ranges::bidirectional_iterator_tag>(),
                        concepts::has_type<I &>(--i),
                        concepts::has_type<I>(i--),
                        concepts::same_type(*i, *i--)
                    ));
            };

            struct RandomAccessIterator
              : refines<BidirectionalIterator, TotallyOrdered>
            {
                template<typename I, typename V = common_reference_t<I>>
                auto requires_(I i) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Derived, category_t<I>, ranges::random_access_iterator_tag>(),
                        concepts::model_of<SignedIntegral>(i - i),
                        concepts::has_type<difference_t<I>>(i - i),
                        concepts::has_type<I>(i + (i - i)),
                        concepts::has_type<I>((i - i) + i),
                        concepts::has_type<I>(i - (i - i)),
                        concepts::has_type<I &>(i += (i-i)),
                        concepts::has_type<I &>(i -= (i - i)),
                        concepts::convertible_to<V>(i[i - i])
                    ));
            };
        }

        template<typename T>
        using Readable = concepts::models<concepts::Readable, T>;

        template<typename Out, typename T>
        using MoveWritable = concepts::models<concepts::MoveWritable, Out, T>;

        template<typename Out, typename T>
        using Writable = concepts::models<concepts::Writable, Out, T>;

        template<typename I, typename O, typename P = ident>
        using IndirectlyMovable = concepts::models<concepts::IndirectlyMovable, I, O, P>;

        template<typename I, typename O, typename P = ident>
        using IndirectlyCopyable = concepts::models<concepts::IndirectlyCopyable, I, O, P>;

        template<typename I1, typename I2>
        using IndirectlySwappable = concepts::models<concepts::IndirectlySwappable, I1, I2>;

        template<typename T>
        using WeaklyIncrementable = concepts::models<concepts::WeaklyIncrementable, T>;

        template<typename T>
        using Incrementable = concepts::models<concepts::Incrementable, T>;

        template<typename I>
        using WeakIterator = concepts::models<concepts::WeakIterator, I>;

        template<typename I>
        using Iterator = concepts::models<concepts::Iterator, I>;

        template<typename Out, typename T>
        using WeakOutputIterator = concepts::models<concepts::WeakOutputIterator, Out, T>;

        template<typename Out, typename T>
        using OutputIterator = concepts::models<concepts::OutputIterator, Out, T>;

        template<typename I>
        using WeakInputIterator = concepts::models<concepts::WeakInputIterator, I>;

        template<typename I>
        using InputIterator = concepts::models<concepts::InputIterator, I>;

        template<typename I>
        using ForwardIterator = concepts::models<concepts::ForwardIterator, I>;

        template<typename I>
        using BidirectionalIterator = concepts::models<concepts::BidirectionalIterator, I>;

        template<typename I>
        using RandomAccessIterator = concepts::models<concepts::RandomAccessIterator, I>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // iterator_concept
        template<typename T>
        using iterator_concept =
            concepts::most_refined<
                meta::list<
                    concepts::RandomAccessIterator,
                    concepts::BidirectionalIterator,
                    concepts::ForwardIterator,
                    concepts::InputIterator,
                    concepts::WeakInputIterator>, T>;

        template<typename T>
        using iterator_concept_t = meta::eval<iterator_concept<T>>;

        // Generally useful to know if an iterator is single-pass or not:
        template<typename I>
        using SinglePass = meta::fast_and<WeakInputIterator<I>, meta::not_<ForwardIterator<I>>>;

        ////////////////////////////////////////////////////////////////////////////////////////////
        // Composite concepts for use defining algorithms:
        template<typename Fun, typename I, typename P = ident,
            typename X = concepts::Invokable::result_t<P, concepts::Readable::value_t<I>>,
            typename Y = concepts::Invokable::result_t<P, concepts::Readable::reference_t<I>>,
            typename Z = concepts::Invokable::result_t<P, concepts::Readable::common_reference_t<I>>>
        using IndirectInvokable1 = meta::fast_and<
            Invokable<Fun, X>,
            Invokable<Fun, Y>,
            Invokable<Fun, Z>,
            CommonReference<
                concepts::Invokable::result_t<Fun, X>,
                concepts::Invokable::result_t<Fun, Y>,
                concepts::Invokable::result_t<Fun, Z>>>;

        template<typename C, typename I0, typename I1 = I0, typename P0 = ident, typename P1 = ident,
            typename X0 = concepts::Invokable::result_t<P0, concepts::Readable::value_t<I0>>,
            typename X1 = concepts::Invokable::result_t<P1, concepts::Readable::value_t<I1>>,
            typename Y0 = concepts::Invokable::result_t<P0, concepts::Readable::reference_t<I0>>,
            typename Y1 = concepts::Invokable::result_t<P1, concepts::Readable::reference_t<I1>>,
            typename Z0 = concepts::Invokable::result_t<P0, concepts::Readable::common_reference_t<I0>>,
            typename Z1 = concepts::Invokable::result_t<P1, concepts::Readable::common_reference_t<I1>>>
        using IndirectInvokable2 = meta::fast_and<
            Invokable<C, X0, X1>,
            Invokable<C, Y0, Y1>,
            Invokable<C, Z0, Z1>,
            Invokable<C, X0, Y1>,
            Invokable<C, Y0, X1>,
            CommonReference<
                concepts::Invokable::result_t<C, X0, X1>,
                concepts::Invokable::result_t<C, Y0, Y1>,
                concepts::Invokable::result_t<C, Z0, Z1>,
                concepts::Invokable::result_t<C, X0, Y1>,
                concepts::Invokable::result_t<C, Y0, X1>>>;

        template<typename C, typename I, typename P = ident,
            typename X = concepts::Invokable::result_t<P, concepts::Readable::value_t<I>>,
            typename Y = concepts::Invokable::result_t<P, concepts::Readable::reference_t<I>>,
            typename Z = concepts::Invokable::result_t<P, concepts::Readable::common_reference_t<I>>>
        using IndirectInvokablePredicate1 = meta::fast_and<
            IndirectInvokable1<P, I>,
            InvokablePredicate<C, X>,
            InvokablePredicate<C, Y>,
            InvokablePredicate<C, Z>>;

        template<typename C, typename I0, typename I1 = I0, typename P0 = ident, typename P1 = ident,
            typename X0 = concepts::Invokable::result_t<P0, concepts::Readable::value_t<I0>>,
            typename X1 = concepts::Invokable::result_t<P1, concepts::Readable::value_t<I1>>,
            typename Y0 = concepts::Invokable::result_t<P0, concepts::Readable::reference_t<I0>>,
            typename Y1 = concepts::Invokable::result_t<P1, concepts::Readable::reference_t<I1>>,
            typename Z0 = concepts::Invokable::result_t<P0, concepts::Readable::common_reference_t<I0>>,
            typename Z1 = concepts::Invokable::result_t<P1, concepts::Readable::common_reference_t<I1>>>
        using IndirectInvokablePredicate2 = meta::fast_and<
            IndirectInvokable1<P0, I0>,
            IndirectInvokable1<P1, I1>,
            InvokablePredicate<C, X0, X1>,
            InvokablePredicate<C, Y0, Y1>,
            InvokablePredicate<C, Z0, Z1>,
            InvokablePredicate<C, X0, Y1>,
            InvokablePredicate<C, Y0, X1>>;

        template<typename C, typename I0, typename I1 = I0, typename P0 = ident, typename P1 = ident,
            typename X0 = concepts::Invokable::result_t<P0, concepts::Readable::value_t<I0>>,
            typename X1 = concepts::Invokable::result_t<P1, concepts::Readable::value_t<I1>>,
            typename Y0 = concepts::Invokable::result_t<P0, concepts::Readable::reference_t<I0>>,
            typename Y1 = concepts::Invokable::result_t<P1, concepts::Readable::reference_t<I1>>,
            typename Z0 = concepts::Invokable::result_t<P0, concepts::Readable::common_reference_t<I0>>,
            typename Z1 = concepts::Invokable::result_t<P1, concepts::Readable::common_reference_t<I1>>>
        using IndirectInvokableRelation = meta::fast_and<
            IndirectInvokable1<P0, I0>,
            IndirectInvokable1<P1, I1>,
            InvokableRelation<C, X0, X1>,
            InvokableRelation<C, Y0, Y1>,
            InvokableRelation<C, Z0, Z1>,
            InvokableRelation<C, X0, Y1>,
            InvokableRelation<C, Y0, X1>>;

        template<typename I, typename V = concepts::Readable::value_t<I>>
        using Permutable = meta::fast_and<
            ForwardIterator<I>,
            Movable<V>,
            IndirectlyMovable<I, I>>;

        template<typename I0, typename I1, typename Out, typename C = ordered_less,
            typename P0 = ident, typename P1 = ident>
        using Mergeable = meta::fast_and<
            InputIterator<I0>,
            InputIterator<I1>,
            WeaklyIncrementable<Out>,
            IndirectInvokableRelation<C, I0, I1, P0, P1>,
            IndirectlyCopyable<I0, Out>,
            IndirectlyCopyable<I1, Out>>;

        template<typename I0, typename I1, typename Out, typename C = ordered_less,
            typename P0 = ident, typename P1 = ident>
        using MergeMovable = meta::fast_and<
            InputIterator<I0>,
            InputIterator<I1>,
            WeaklyIncrementable<Out>,
            IndirectInvokableRelation<C, I0, I1, P0, P1>,
            IndirectlyMovable<I0, Out>,
            IndirectlyMovable<I1, Out>>;

        template<typename I, typename C = ordered_less, typename P = ident>
        using Sortable = meta::fast_and<
            ForwardIterator<I>,
            IndirectInvokableRelation<C, I, I, P, P>,
            Permutable<I>>;

        template<typename I, typename V2, typename C = ordered_less, typename P = ident>
        using BinarySearchable = meta::fast_and<
            ForwardIterator<I>,
            TotallyOrdered<V2>,
            IndirectInvokableRelation<C, I, V2 const *, P, ident>>;

        template<typename I1, typename I2, typename C = equal_to, typename P1 = ident,
            typename P2 = ident>
        using WeaklyAsymmetricallyComparable = meta::fast_and<
            InputIterator<I1>,
            WeakInputIterator<I2>,
            IndirectInvokablePredicate2<C, I1, I2, P1, P2>>;

        template<typename I1, typename I2, typename C = equal_to, typename P1 = ident,
            typename P2 = ident>
        using AsymmetricallyComparable = meta::fast_and<
            WeaklyAsymmetricallyComparable<I1, I2, C, P1, P2>,
            InputIterator<I2>>;

        template<typename I1, typename I2, typename C = equal_to, typename P1 = ident,
            typename P2 = ident>
        using WeaklyComparable = meta::fast_and<
            WeaklyAsymmetricallyComparable<I1, I2, C, P1, P2>,
            IndirectInvokableRelation<C, I1, I2, P1, P2>>;

        template<typename I1, typename I2, typename C = equal_to, typename P1 = ident,
            typename P2 = ident>
        using Comparable = meta::fast_and<
            WeaklyComparable<I1, I2, C, P1, P2>,
            InputIterator<I2>>;

        namespace concepts
        {
            struct IteratorRange
            {
                template<typename I, typename S>
                auto requires_(I i, S s) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Iterator, I>(),
                        concepts::model_of<Regular, S>(),
                        concepts::model_of<EqualityComparable, I, S>()
                    ));
            };

            // Detail, used only to constrain common_iterator::operator-, which
            // is used by SizedIteratorRange
            struct SizedIteratorRangeLike_
              : refines<IteratorRange>
            {
              template<typename I, typename S>
                auto requires_(I i, S s) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Integral>(s - i),
                        concepts::same_type(s - i, i - s)
                    ));
            };

            struct SizedIteratorRange
              : refines<IteratorRange>
            {
                template<typename I, typename S,
                    enable_if_t<std::is_same<I, S>::value> = 0>
                auto requires_(I i, I s) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<Integral>(s - i)
                    ));

                template<typename I, typename S,
                    enable_if_t<!std::is_same<I, S>::value> = 0,
                    typename C = common_type_t<I, S>>
                auto requires_(I i, S s) -> decltype(
                    concepts::valid_expr(
                        concepts::model_of<SizedIteratorRange, I, I>(),
                        concepts::model_of<Common, I, S>(),
                        concepts::model_of<SizedIteratorRange, C, C>(),
                        concepts::model_of<Integral>(s - i),
                        concepts::same_type(s - i, i - s)
                    ));
            };
        }

        template<typename I, typename S>
        using IteratorRange = concepts::models<concepts::IteratorRange, I, S>;

        template<typename I, typename S>
        using SizedIteratorRangeLike_ = concepts::models<concepts::SizedIteratorRangeLike_, I, S>;

        template<typename I, typename S>
        using SizedIteratorRange = concepts::models<concepts::SizedIteratorRange, I, S>;

        template<typename I, typename S = I>
        using sized_iterator_range_concept =
            concepts::most_refined<
                meta::list<
                    concepts::SizedIteratorRange,
                    concepts::IteratorRange>, I, S>;

        template<typename I, typename S = I>
        using sized_iterator_range_concept_t = meta::eval<sized_iterator_range_concept<I, S>>;
        /// @}
    }
}

#endif // RANGES_V3_UTILITY_ITERATOR_CONCEPTS_HPP
