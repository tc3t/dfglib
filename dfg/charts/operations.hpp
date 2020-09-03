#include "../dfgDefs.hpp"
#include "../cont/MapVector.hpp"
#include "../cont/Vector.hpp"
#include "../SzPtrTypes.hpp"
#include "../str/string.hpp"
#include "commonChartTools.hpp"
#include "../dfgAssert.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <vector>
#include "../dataAnalysis/smoothWithNeighbourAverages.hpp"
#include "../dataAnalysis/smoothWithNeighbourMedians.hpp"

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(charts) {

    // Data object that operations receive and pass on; more or less simply a list of vectors.
    //  -m_vectorRefs defines vector count and each of its element has variant wrapper (DataVectorRef)
    //  -If operation needs e.g. to create new data, there are internal buffers that can be used (m_valueVectors, m_stringVectors).
    //      -Typically m_vectorRefs has pointers to externally owned data.
    class ChartOperationPipeData
    {
    public:
        using StringVector = ::DFG_MODULE_NS(cont)::Vector<StringUtf8>;

        // Variant-like wrapper for vector data.
        class DataVectorRef
        {
        public:

            DataVectorRef(std::nullptr_t)
            {}

            DataVectorRef(ValueVectorD* p = nullptr) :
                m_pValueVector(p)
            {}

            DataVectorRef(const ValueVectorD* p) :
                m_pConstValueVector(p)
            {}

            DataVectorRef(StringVector* p) :
                m_pStringVector(p)
            {}

            DataVectorRef(const StringVector* p) :
                m_pConstStringVector(p)
            {}

            bool isNull() const
            {
                return m_pValueVector == nullptr && m_pStringVector == nullptr && m_pConstValueVector == nullptr && m_pConstStringVector == nullptr;
            }

                  ValueVectorD* values()             { return m_pValueVector; }
            const ValueVectorD* constValues() const  { return (m_pConstValueVector) ? m_pConstValueVector : m_pValueVector; }
                  StringVector* strings()            { return m_pStringVector; }
            const StringVector* constStrings() const { return (m_pConstStringVector) ? m_pConstStringVector : m_pStringVector; }

                  ValueVectorD* m_pValueVector       = nullptr;
            const ValueVectorD* m_pConstValueVector  = nullptr; // TODO: this should be span, i.e. if referencing external data, it shouldn't need to be stored as ValueVectorD
                  StringVector* m_pStringVector      = nullptr;
            const StringVector* m_pConstStringVector = nullptr;
        };

        ChartOperationPipeData() = default;

        template <class X_T, class Y_T>
        ChartOperationPipeData(X_T&& xvals, Y_T&& yvals);

        // Creates internal string vectors.
        void createStringVectors(size_t nCount, size_t nVectorSize);

        // Fills internal string vector as tranform from iterable. This does not create vectors so will do nothing if index is invalid.
        template <class Iterable_T, class Func_T>
        void fillStringVector(const size_t nPos, const Iterable_T& iterable, Func_T func);

        // Overload for double input transform.
        template <class Iterable1_T, class Iterable2_T, class Func_T>
        void fillStringVector(const size_t nPos, const Iterable1_T& iterable1, const Iterable2_T iterable2, Func_T func);

        // Sets internal value vectors as active data.
        void setValueVectorsAsData();
        // Sets internal string vectors as active data.
        void setStringVectorsAsData();

        template <class Return_T, class ModAccess_T, class ConstAccess_T, class EditableAccess_T>
        Return_T privItemsByDataVectorRef(DataVectorRef& ref, ModAccess_T&& modAccess, ConstAccess_T&& constAccess, EditableAccess_T&& editableAccess);

        // Returns i'th vector as ValueVectorD if i'th vector exists and is of value type.
        ValueVectorD* valuesByIndex(size_t n);
        ValueVectorD* valuesByRef(DataVectorRef& ref);
        // Returns i'th vector as const ValueVectorD if i'th vector exists and is of value type.
        const ValueVectorD* constValuesByIndex(size_t n) const;
        // Returns i'th vector as StringVector if i'th vector exists and is of string type.
        StringVector* stringsByIndex(size_t n);
        StringVector* stringsByRef(DataVectorRef& ref);
        const StringVector* constStringsByIndex(size_t n) const;

        // Returns editable value vector, creates new one if such is not yet available. Returned vector might be
        // one from active data vectors
        ValueVectorD* editableValuesByIndex(size_t i);
        ValueVectorD* editableValuesByRef(DataVectorRef& ref);

        StringVector* editableStringsByRef(DataVectorRef& ref);

        // Sets active vectors.
        void setDataRefs(DataVectorRef ref0, DataVectorRef ref1 = DataVectorRef(), DataVectorRef ref2 = DataVectorRef());

        // Returns the number of active vectors
        size_t vectorCount() const;

        // Sets size of all active non-null vectors to zero.
        void setVectorSizesToZero();

        std::vector<DataVectorRef> m_vectorRefs;    // Stores references to data vectors and defines the actual vector count that this data has.
        // Note: using deque to avoid invalidation of resize.
        std::deque<ValueVectorD> m_valueVectors;    // Temporary value buffers that can be used e.g. if operation changes data type and can't edit existing.
        std::deque<StringVector> m_stringVectors;   // Temporary string buffers that can be used e.g. if operation changes data type and can't edit existing.
    };

    template <class X_T, class Y_T>
    inline ChartOperationPipeData::ChartOperationPipeData(X_T&& xvals, Y_T&& yvals)
    {
        setDataRefs(std::forward<X_T>(xvals), std::forward<Y_T>(yvals));
    }

    inline size_t ChartOperationPipeData::vectorCount() const
    {
        return m_vectorRefs.size();
    }

    // Sets size of all active vectors to zero.
    inline void ChartOperationPipeData::setVectorSizesToZero()
    {
        for (auto& item : m_vectorRefs)
        {
            if (item.isNull())
                continue;
            if (item.values() || item.constValues())
            {
                auto pValues = editableValuesByRef(item);
                if (pValues)
                {
                    pValues->clear();
                    item = DataVectorRef(pValues);
                }
                else
                    item = DataVectorRef();
            }
            else if (item.strings() || item.constStrings())
            {
                auto pStrings = editableStringsByRef(item);
                if (pStrings)
                {
                    pStrings->clear();
                    item = DataVectorRef(pStrings);
                }
                else
                    item = DataVectorRef();
            }
            else
            {
                DFG_ASSERT_IMPLEMENTED(false);
            }
        }
    }

    inline void ChartOperationPipeData::createStringVectors(size_t nCount, size_t nVectorSize)
    {
        m_stringVectors.resize(nCount, std::vector<StringUtf8>(nVectorSize));
    }

    template <class Iterable_T, class Func_T>
    inline void ChartOperationPipeData::fillStringVector(const size_t nPos, const Iterable_T & iterable, Func_T func)
    {
        if (!isValidIndex(m_stringVectors, nPos))
            return;
        auto& v = m_stringVectors[nPos];
        v.resize(iterable.size());
        std::transform(iterable.begin(), iterable.end(), v.begin(), func);
    }

    template <class Iterable1_T, class Iterable2_T, class Func_T>
    inline void ChartOperationPipeData::fillStringVector(const size_t nPos, const Iterable1_T & iterable1, const Iterable2_T iterable2, Func_T func)
    {
        if (!isValidIndex(m_stringVectors, nPos) || iterable1.size() != iterable2.size())
            return;
        auto& v = m_stringVectors[nPos];
        v.resize(iterable1.size());
        std::transform(iterable1.begin(), iterable1.end(), iterable2.begin(), v.begin(), func);
    }

    inline void ChartOperationPipeData::setValueVectorsAsData()
    {
        m_vectorRefs.resize(m_valueVectors.size());
        std::transform(m_valueVectors.begin(), m_valueVectors.end(), m_vectorRefs.begin(), [](ValueVectorD& v) { return DataVectorRef(&v); });
    }

    inline void ChartOperationPipeData::setStringVectorsAsData()
    {
        m_vectorRefs.resize(m_stringVectors.size());
        std::transform(m_stringVectors.begin(), m_stringVectors.end(), m_vectorRefs.begin(), [](StringVector& v) { return DataVectorRef(&v); });
    }

    template <class Return_T, class ModAccess_T, class ConstAccess_T, class EditableAccess_T>
    Return_T ChartOperationPipeData::privItemsByDataVectorRef(DataVectorRef& item, ModAccess_T&& modAccess, ConstAccess_T&& constAccess, EditableAccess_T&& editableAccess)
    {
        if (modAccess(item))
            return modAccess(item);
        // Didn't have modifiable items, checking if there are const items.
        auto pConstValues = constAccess(item);
        if (!pConstValues)
            return nullptr;
        // Getting here means that there is const items. Must make a modifiable copy.
        auto pValues = editableAccess(item);
        if (pValues)
        {
            *pValues = *pConstValues;
            item = pValues;
        }
        return pValues;
    }

    inline auto ChartOperationPipeData::valuesByRef(DataVectorRef& ref) -> ValueVectorD*
    {
        return privItemsByDataVectorRef<ValueVectorD*>(
            ref,
            [](DataVectorRef& ref)  { return ref.values(); },
            [](DataVectorRef& ref)  { return ref.constValues(); },
            [&](DataVectorRef& ref) { return editableValuesByRef(ref); });
    }

    // Returns values by index. If index has const-values, make a copy and returns reference to copied data.
    inline auto ChartOperationPipeData::valuesByIndex(size_t n) -> ValueVectorD*
    {
        if (isValidIndex(m_vectorRefs, n))
            return valuesByRef(m_vectorRefs[n]);
        else
            return nullptr;
    }

    inline auto ChartOperationPipeData::constValuesByIndex(size_t n) const -> const ValueVectorD*
    {
        return (isValidIndex(m_vectorRefs, n)) ? m_vectorRefs[n].constValues() : nullptr;
    }

    inline auto ChartOperationPipeData::stringsByRef(DataVectorRef& ref) -> StringVector*
    {
        return privItemsByDataVectorRef<StringVector*>(
            ref,
            [](DataVectorRef& ref)  { return ref.strings(); },
            [](DataVectorRef& ref)  { return ref.constStrings(); },
            [&](DataVectorRef& ref) { return editableStringsByRef(ref); });
    }

    inline auto ChartOperationPipeData::stringsByIndex(size_t n) -> StringVector*
    {
        if (isValidIndex(m_vectorRefs, n))
            return stringsByRef(m_vectorRefs[n]);
        else
            return nullptr;
    }

    inline auto ChartOperationPipeData::constStringsByIndex(size_t n) const -> const StringVector*
    {
        if (isValidIndex(m_vectorRefs, n))
            return m_vectorRefs[n].constStrings();
        else
            return nullptr;
    }

    inline auto ChartOperationPipeData::editableValuesByRef(DataVectorRef& ref) -> ValueVectorD*
    {
        return editableValuesByIndex(static_cast<size_t>(&ref - &m_vectorRefs.front()));
    }

    inline auto ChartOperationPipeData::editableValuesByIndex(const size_t i) -> ValueVectorD*
    {
        if (i >= m_valueVectors.max_size())
            return nullptr;
        if (!isValidIndex(m_valueVectors, i))
            m_valueVectors.resize(i + 1);
        return &m_valueVectors[i];
    }

    inline auto ChartOperationPipeData::editableStringsByRef(DataVectorRef& ref) -> StringVector*
    {
        const auto nIndex = static_cast<size_t>(&ref - &m_vectorRefs.front());
        if (nIndex >= m_valueVectors.max_size())
            return nullptr;
        if (!isValidIndex(m_stringVectors, nIndex))
            m_stringVectors.resize(nIndex + 1);
        return &m_stringVectors[nIndex];
    }

    inline void ChartOperationPipeData::setDataRefs(DataVectorRef ref0, DataVectorRef ref1, DataVectorRef ref2)
    {
        m_vectorRefs.clear();
        std::array<DataVectorRef, 3> refs = {ref0, ref1, ref2};
        if (!ref2.isNull())
            m_vectorRefs.resize(3);
        else if (!ref1.isNull())
            m_vectorRefs.resize(2);
        else if (!ref0.isNull())
            m_vectorRefs.resize(1);
        std::copy(refs.begin(), refs.begin() + m_vectorRefs.size(), m_vectorRefs.begin());
    }

    class CreateOperationArgs : public ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem
    {
    public:
        using BaseClass = ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem;
        using StringToDoubleConverter = std::function<double(const StringViewSzUtf8&)>;
        

        CreateOperationArgs() = default;

        CreateOperationArgs(const BaseClass& other) : BaseClass(other) {  }
        CreateOperationArgs(BaseClass&& other)      : BaseClass(std::move(other)) {  }

        template <class T>
        T valueAs(const size_t nIndex) const
        {
            DFG_STATIC_ASSERT(std::is_floating_point<T>::value, "Current implementation assumes floating point target");
            if (m_stringToDoubleConverter)
                return m_stringToDoubleConverter(value(nIndex));
            else
            {
                bool bOk;
                const auto v = ::DFG_MODULE_NS(str)::strTo<T>(value(nIndex), &bOk);
                return (bOk) ? v : std::numeric_limits<T>::quiet_NaN();
            }
        }

        StringToDoubleConverter m_stringToDoubleConverter;
    };

    // Operation object. Operations are defined by creating object of this type and passing callable to constructor.
    class ChartEntryOperation
    {
    public:
        using CreationArgList   = CreateOperationArgs;
        using StringView        = CreationArgList::StringView;
        using StringViewSz      = CreationArgList::StringViewSz;
        using StringT           = StringView::StringT;
        using DefinitionArgList = std::vector<double>;
        static void defaultCall(ChartEntryOperation&, ChartOperationPipeData&);
        using OperationCall     = decltype(ChartEntryOperation::defaultCall);
        using DataVectorRef     = ChartOperationPipeData::DataVectorRef;

        // Error flags; set when errors are encountered.
        using ErrorMask = int;
        enum Error
        {
            error_missingInput                  = 0x1,    // Pipe data does not have expected data.
            error_noCallable                    = 0x2,    // Operation does not have callable.
            error_badCreationArgs               = 0x4,    // Arguments used to define operation are invalid.
            error_pipeDataVectorSizeMismatch    = 0x8,    // Input vectors have different size.
            error_unexpectedInputVectorCount    = 0x10,   // Input has unexpected vector count.
            error_unexpectedInputVectorTypes    = 0x20,   // Input has unexpected type (e.g. string instead of number)
            error_unableToCreateDataVectors     = 0x40    // Operation failed because new data vectors couldn't be created.
        };

        // Helper enum for storing creation arg string as numeric value.
        enum AxisIndex
        {
            axisIndex_invalid   = -1,
            axisIndex_x         = 0,
            axisIndex_y         = 1
        };

        ChartEntryOperation(OperationCall call = nullptr);

        // Operation evaluates to true bool iff it has non-default call implementation.
        explicit operator bool() const;

        // Executes operation.
        void operator()(ChartOperationPipeData& arg);

        // Sets given error
        void setError(Error error);

        // Returns true if any error flag is on.
        bool hasErrors() const;
        // Checks if given error flag is on.
        bool hasError(Error) const;

        // Calls 'func' on each vector in pipe data.
        template <class Func_T>
        void forEachVector(ChartOperationPipeData& arg, Func_T&& func);

        // Helper that converts axis string representation to axis index.
        static double axisStrToIndex(const StringViewSz& sv);
        // Converts double-valued axis index to integer valued axis index.
        static size_t floatAxisValueToAxisIndex(double index);

        // Filters all pipe data vectors by keep flags created by input vector 'axis' and keep predicate 'keepPred'.
        // Concrete example: filters x,y vectors by pass window applied to x-axis, where 'keepPred' is called for
        //                   for all x-values and it returns true if x-value is within window and should be kept.
        template <class Func_T>
        void privFilterBySingle(ChartOperationPipeData& arg, const double axis, Func_T&& keepPred);

        // Returns pointer to pipe vector by axis index.
        static ValueVectorD* privPipeVectorByAxisIndex(ChartOperationPipeData& arg, double index);

        // Returns ChartEntryOperation() from case that creation args were invalid.
        static ChartEntryOperation privInvalidCreationArgsResult();

        OperationCall*    m_pCall          = &ChartEntryOperation::defaultCall;
        ErrorMask         m_errors         = 0;
        DefinitionArgList m_argList;
        StringT           m_sDefinition;            // For (optionally) storing the text from which operation was created from.
    }; // ChartEntryOperation

    inline ChartEntryOperation::ChartEntryOperation(OperationCall call)
        : m_pCall(call)
    {
        if (!m_pCall)
            m_pCall = &ChartEntryOperation::defaultCall;
    }

    inline void ChartEntryOperation::setError(const Error errorMask)
    {
        m_errors |= errorMask;
    }

    inline bool ChartEntryOperation::hasErrors() const
    {
        return m_errors != 0;
    }

    inline bool ChartEntryOperation::hasError(Error err) const
    {
        return (m_errors & err) != 0;
    }

    inline auto ChartEntryOperation::privPipeVectorByAxisIndex(ChartOperationPipeData& arg, const double index) -> ValueVectorD*
    {
        return arg.valuesByIndex(floatAxisValueToAxisIndex(index));
    }

    template <class Func_T>
    void ChartEntryOperation::forEachVector(ChartOperationPipeData& arg, Func_T&& func)
    {
        ::DFG_MODULE_NS(alg)::forEachFwd(arg.m_vectorRefs, func);
    }

    template <class Func_T>
    inline void ChartEntryOperation::privFilterBySingle(ChartOperationPipeData& arg, const double axis, Func_T && func)
    {
        using namespace DFG_MODULE_NS(alg);
        auto pCont = privPipeVectorByAxisIndex(arg, axis);
        if (!pCont)
        {
            const auto nIndex = floatAxisValueToAxisIndex(axis);
            if (isValidIndex(arg.m_vectorRefs, nIndex) && !arg.m_vectorRefs[nIndex].isNull())
                setError(error_unexpectedInputVectorTypes);
            else
                setError(error_missingInput);
            arg.setVectorSizesToZero();
            return;
        }
        // Creating keep-flags
        std::vector<bool> keepFlags;
        keepFlags.resize(pCont->size());
        std::transform(pCont->begin(), pCont->end(), keepFlags.begin(), [&](const double d) { return func(d); });
        // Filtering all vectors by keep-flags.
        forEachVector(arg, [&](DataVectorRef& ref)
        {
            auto pValues = arg.valuesByRef(ref);
            if (pValues)
            {
                keepByFlags(*pValues, keepFlags);
                return;
            }
            auto pStrings = arg.stringsByRef(ref);
            if (pStrings)
            {
                keepByFlags(*pStrings, keepFlags);
                return;
            }
            if (!ref.isNull())
                this->setError(error_unexpectedInputVectorTypes);
            DFG_ASSERT_IMPLEMENTED(false);
        });
    }

    inline ChartEntryOperation::operator bool() const
    {
        return m_pCall != &ChartEntryOperation::defaultCall;
    }

    inline void ChartEntryOperation::defaultCall(ChartEntryOperation& op, ChartOperationPipeData&)
    {
        op.setError(error_noCallable);
    }

    inline void ChartEntryOperation::operator()(ChartOperationPipeData& arg)
    {
        DFG_ASSERT_UB(m_pCall != nullptr);
        m_pCall(*this, arg);
    }

    inline double ChartEntryOperation::axisStrToIndex(const StringViewSz& sv)
    {
        if (sv == DFG_UTF8("x"))
            return axisIndex_x;
        else if (sv == DFG_UTF8("y"))
            return axisIndex_y;
        else
            return axisIndex_invalid;
    }

    inline auto ChartEntryOperation::floatAxisValueToAxisIndex(double index) -> size_t
    {
        return (index >= 0 && index < 10000 && ::DFG_MODULE_NS(math)::isIntegerValued(index)) ? static_cast<size_t>(index) : std::numeric_limits<size_t>::max();
    }

    // Returns ChartEntryOperation() from case that creation args were invalid.
    inline auto ChartEntryOperation::privInvalidCreationArgsResult() -> ChartEntryOperation
    {
        ChartEntryOperation op;
        op.setError(error_badCreationArgs);
        return op;
    }

    // Stores operations as id -> operation map.
    class ChartEntryOperationManager
    {
    public:
        using CreationArgList       = ChartEntryOperation::CreationArgList;
        using CreatorFunc           = std::function<ChartEntryOperation(const CreationArgList&)>;
        using ParenthesisItem       = ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem;

        ChartEntryOperationManager();

        // Adds creator mapping, returns true if added, false otherwise (may happen e.g. if id is already present)
        bool add(StringViewUtf8 svId, CreatorFunc creator);

        // Convenience function for add operation from class that has id() and create() defined.
        template <class Operation_T>
        bool add();

        ChartEntryOperation createOperation(StringViewUtf8);

        void setStringToDoubleConverter(CreationArgList::StringToDoubleConverter converter);

        bool hasOperation(StringViewUtf8 svId) const;

        template <class Func_T>
        void forEachOperationId(Func_T&& func) const;

        size_t operationCount() const { return m_knownOperations.size(); }

        DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, CreatorFunc> m_knownOperations;
        CreationArgList::StringToDoubleConverter m_stringToDoubleConverter;
    }; // class ChartEntryOperationManager

    inline bool ChartEntryOperationManager::add(StringViewUtf8 svId, CreatorFunc creator)
    {
        if (!creator)
        {
            DFG_ASSERT_CORRECTNESS(false);
            return false;
        }
        auto insertRv = m_knownOperations.insert(svId.toString(), std::move(creator));
        return !insertRv.second;
    }

    template <class Operation_T>
    inline bool ChartEntryOperationManager::add()
    {
        return add(Operation_T::id(), Operation_T::create);
    }

    inline bool ChartEntryOperationManager::hasOperation(StringViewUtf8 svId) const
    {
        return this->m_knownOperations.hasKey(svId);
    }

    template <class Func_T>
    inline void ChartEntryOperationManager::forEachOperationId(Func_T&& func) const
    {
        for (const auto& item : this->m_knownOperations)
        {
            func(item.first);
        }
    }


namespace operations
{
    /** Implements pass window operation
     *      Filters out all data points (xi, yi) where chosen coordinate (x/y) is not within [a, b]
     *  Id:
     *      passWindow
     *  Parameters:
     *      -0: axis, either x or y
     *      -1: pass window lower limit
     *      -2: pass window upper limit
     *  Dependencies
     *      -[x] or [y] depending on parameter 0
     *  Outputs:
     *      -The same number of vectors as in input
     *  Details:
     *      -NaN handling: unspecified
     */
    class PassWindowOperation : public ChartEntryOperation
    {
    public:
        // Returns operation id
        static SzPtrUtf8R id()
        {
            return DFG_UTF8("passWindow");
        }

        // Returns PassWindowOperation or invalid operation eg. if arguments are invalid.
        static ChartEntryOperation create(const CreateOperationArgs& argList)
        {
            if (argList.valueCount() != 3)
                return privInvalidCreationArgsResult();
            const auto axis = axisStrToIndex(argList.value(0));
            if (axis == axisIndex_invalid)
                return privInvalidCreationArgsResult();
            ChartEntryOperation op(&PassWindowOperation::operation);

            op.m_argList.resize(3);
            op.m_argList[0] = axis;
            op.m_argList[1] = argList.valueAs<double>(1);
            op.m_argList[2] = argList.valueAs<double>(2);
            return op;
        }

        // Executes operation on pipe data.
        static void operation(ChartEntryOperation& op, ChartOperationPipeData& arg)
        {
            const auto& argList = op.m_argList;
            if (argList.size() < 3)
            {
                op.setError(error_badCreationArgs);
                return;
            }
            const auto axis = argList[0];
            const auto lowerBound = argList[1];
            const auto upperBound = argList[2];
            op.privFilterBySingle(arg, axis, [=](const double v)
            {
                return v >= lowerBound && v <= upperBound;
            });
        }
    }; // PassWindowOperation


 /** Implements index neighbour smoothing
  *
  *  Id:
  *      smoothing_indexNb
  *  Parameters:
  *      -[0]: index radius for number of neighbours to include, default = 1 = one neighbour from both sides. 0 = no neighbours = does nothing
  *      -[1]: smoothing type, default average. Possible values: average, median, default
  *  Dependencies
  *      -[y]
  *  Outputs:
  *      -[x] unmodified, [y] with smoothing applied
  *  Details:
  *      -If input vector count != 2, considered as error.
  *      -If either side has less than 'radius' neighbours at some point, smoothing may be unbalanced, i.e. takes less neighbours from other size.
  */
    class Smoothing_indexNb : public ChartEntryOperation
    {
    public:
        static SzPtrUtf8R id();

        static ChartEntryOperation create(const CreationArgList& argList);

        static constexpr double smoothingTypeAverage() { return 0; }
        static constexpr double smoothingTypeMedian() { return 1; }

        // Executes operation on pipe data.
        static void operation(ChartEntryOperation& op, ChartOperationPipeData& arg);
    }; // class Smoothing_indexNb

    inline auto Smoothing_indexNb::id() -> SzPtrUtf8R
    {
        return DFG_UTF8("smoothing_indexNb");
    }

    inline auto Smoothing_indexNb::create(const CreationArgList& argList) -> ChartEntryOperation
    {
        if (argList.valueCount() >= 3)
            return ChartEntryOperation();
        const double arg0 = (argList.valueCount() >= 1) ? argList.valueAs<double>(0) : 1;
        if (!::DFG_MODULE_NS(math)::isFloatConvertibleTo<uint32>(arg0))
            return ChartEntryOperation();
        double arg1 = smoothingTypeAverage();
        if (argList.valueCount() >= 2)
        {
            const auto sArg1 = argList.value(1);
            if (sArg1 == DFG_UTF8("median"))
                arg1 = smoothingTypeMedian();
            else if (sArg1 != DFG_UTF8("default") && sArg1 != DFG_UTF8("average"))
                return ChartEntryOperation();
        }
        
        ChartEntryOperation op(&Smoothing_indexNb::operation);
        op.m_argList.resize(2);
        op.m_argList[0] = arg0;
        op.m_argList[1] = arg1;
        return op;
    }

    // Executes operation on pipe data.
    inline void Smoothing_indexNb::operation(ChartEntryOperation& op, ChartOperationPipeData& arg)
    {
        const auto& argList = op.m_argList;
        size_t nNbRadius = 0;
        if (argList.size() < 1 || !::DFG_MODULE_NS(math)::isFloatConvertibleTo(argList[0], &nNbRadius))
        {
            op.setError(error_badCreationArgs);
            return;
        }
        if (arg.vectorCount() != 2)
        {
            op.setError(error_unexpectedInputVectorCount);
            return;
        }
        if (nNbRadius == 0)
            return;
        auto pY = arg.valuesByIndex(1);
        if (!pY)
        {
            op.setError(error_unexpectedInputVectorTypes);
            return;
        }
        const auto smoothingType = (argList.size() >= 2) ? argList[1] : smoothingTypeAverage();
        if (smoothingType == smoothingTypeAverage())
            ::DFG_MODULE_NS(dataAnalysis)::smoothWithNeighbourAverages(*pY, nNbRadius);
        else if (smoothingType == smoothingTypeMedian())
            ::DFG_MODULE_NS(dataAnalysis)::smoothWithNeighbourMedians(*pY, nNbRadius);
        else
            op.setError(error_badCreationArgs);
    }

} // namespace operations

inline ChartEntryOperationManager::ChartEntryOperationManager()
{
    // Adding built-in operations.
    add<operations::PassWindowOperation>();
    add<operations::Smoothing_indexNb>();
}

inline auto ChartEntryOperationManager::createOperation(StringViewUtf8 svFuncAndParams) -> ChartEntryOperation
{
    CreateOperationArgs args = ParenthesisItem::fromStableView(svFuncAndParams);
    if (args.key().empty())
        return ChartEntryOperation();
    args.m_stringToDoubleConverter = m_stringToDoubleConverter;
    auto iter = m_knownOperations.find(args.key());
    if (iter != m_knownOperations.end())
        return iter->second(args);

    // No requested operation found.
    return ChartEntryOperation();
}

inline void ChartEntryOperationManager::setStringToDoubleConverter(CreationArgList::StringToDoubleConverter converter)
{
    m_stringToDoubleConverter = converter;
}

class ChartEntryOperationList : public ::DFG_MODULE_NS(cont)::Vector<ChartEntryOperation>
{
public:
    using BaseClass = ::DFG_MODULE_NS(cont)::Vector<ChartEntryOperation>;
    using BaseClass::BaseClass; // Inheriting constructor

    void executeAll(ChartOperationPipeData& pipeData);
};



}} // module namespace

inline void ::DFG_MODULE_NS(charts)::ChartEntryOperationList::executeAll(ChartOperationPipeData& pipeData)
{
    for (auto& op : (*this))
    {
        op(pipeData);
    }
}
