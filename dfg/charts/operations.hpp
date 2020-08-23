#include "../dfgDefs.hpp"
#include "../cont/MapVector.hpp"
#include "../SzPtrTypes.hpp"
#include "../str/string.hpp"
#include "commonChartTools.hpp"
#include "../dfgAssert.hpp"
#include <algorithm>
#include <deque>
#include <functional>
#include <vector>

DFG_ROOT_NS_BEGIN{ DFG_SUB_NS(charts) {

    // Data object that operations receive and pass on; more or less simply a list of vectors.
    //  -m_vectorRefs defines vector count and each of its element has variant wrapper (DataVectorRef)
    //  -If operation needs e.g. to create new data, there are internal buffers that can be used (m_valueVectors, m_stringVectors).
    //      -Typically m_vectorRefs has pointers to externally owned data.
    class ChartOperationPipeData
    {
    public:
        using StringVector = std::vector<StringUtf8>;

        // Variant-like wrapper for vector data.
        class DataVectorRef
        {
        public:
            DataVectorRef(ValueVectorD* p = nullptr) :
                m_pValueVector(p)
            {}
            DataVectorRef(StringVector* p) :
                m_pStringVector(p)
            {}

            bool isNull() const
            {
                return m_pValueVector == nullptr && m_pStringVector == nullptr;
            }

            ValueVectorD* values() { return m_pValueVector; }
            StringVector* strings() { return m_pStringVector; }

            ValueVectorD* m_pValueVector  = nullptr;
            StringVector* m_pStringVector = nullptr;
        };


        ChartOperationPipeData(ValueVectorD* pXvals, ValueVectorD* pYvals);

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

        // Returns i'th vector as ValueVectorD if i'th vector exists and is of value type.
        ValueVectorD* valuesByIndex(size_t n);
        // Returns i'th vector as StringVector if i'th vector exists and is of string type.
        StringVector* stringsByIndex(size_t n);

        // Returns editable value vector, creates new one if such is not yet available. Returned vector might be
        // one from active data vectors
        ValueVectorD* editableValuesByIndex(size_t i);

        // Sets active vectors.
        void setDataRefs(DataVectorRef ref0, DataVectorRef ref1 = DataVectorRef(), DataVectorRef ref2 = DataVectorRef());

        // Returns the number of active vectors
        size_t vectorCount() const;

        std::vector<DataVectorRef> m_vectorRefs;    // Stores references to data vectors and defines the actual vector count that this data has.
        // Note: using deque to avoid invalidation of resize.
        std::deque<ValueVectorD> m_valueVectors;    // Temporary value buffers that can be used e.g. if operation changes data type and can't edit existing.
        std::deque<StringVector> m_stringVectors;   // Temporary string buffers that can be used e.g. if operation changes data type and can't edit existing.
    };

    inline ChartOperationPipeData::ChartOperationPipeData(ValueVectorD * pXvals, ValueVectorD * pYvals)
    {
        setDataRefs(pXvals, pYvals);
    }

    inline size_t ChartOperationPipeData::vectorCount() const
    {
        return m_vectorRefs.size();
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

    inline auto ChartOperationPipeData::valuesByIndex(size_t n) -> ValueVectorD*
    {
        return (isValidIndex(m_vectorRefs, n)) ? m_vectorRefs[n].values() : nullptr;
    }

    inline auto ChartOperationPipeData::stringsByIndex(size_t n) -> StringVector*
    {
        return (isValidIndex(m_vectorRefs, n)) ? m_vectorRefs[n].strings() : nullptr;
    }

    inline auto ChartOperationPipeData::editableValuesByIndex(size_t i) -> ValueVectorD*
    {
        if (i >= m_valueVectors.max_size())
            return nullptr;
        if (!isValidIndex(m_valueVectors, i))
            m_valueVectors.resize(i + 1);
        return &m_valueVectors[i];
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

    // Operation object. Operations are defined by creating object of this type and passing callable to constructor.
    class ChartEntryOperation
    {
    public:
        using CreationArgList   = ::DFG_MODULE_NS(charts)::DFG_DETAIL_NS::ParenthesisItem;
        using StringView        = CreationArgList::StringView;
        using DefinitionArgList = std::vector<double>;
        static void defaultCall(ChartEntryOperation&, ChartOperationPipeData&);
        using OperationCall     = decltype(ChartEntryOperation::defaultCall);
        using DataVectorRef     = ChartOperationPipeData::DataVectorRef;

        // Error flags; set when errors are ncounted.
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
        static double axisStrToIndex(const StringView& sv);

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

    inline auto ChartEntryOperation::privPipeVectorByAxisIndex(ChartOperationPipeData & arg, const double index) -> ValueVectorD*
    {
        if (index == axisIndex_x)
            return arg.valuesByIndex(0);
        else if (index == axisIndex_y)
            return arg.valuesByIndex(1);
        else
            return nullptr;
    }

    template <class Func_T>
    void ChartEntryOperation::forEachVector(ChartOperationPipeData& arg, Func_T&& func)
    {
        ::DFG_MODULE_NS(alg)::forEachFwd(arg.m_vectorRefs, func);
    }

    template <class Func_T>
    inline void ChartEntryOperation::privFilterBySingle(ChartOperationPipeData & arg, const double axis, Func_T && func)
    {
        using namespace DFG_MODULE_NS(alg);
        auto pCont = privPipeVectorByAxisIndex(arg, axis);
        if (!pCont)
        {
            setError(error_missingInput);
            return;
        }
        // Creating keep-flags
        std::vector<bool> keepFlags;
        keepFlags.resize(pCont->size());
        std::transform(pCont->begin(), pCont->end(), keepFlags.begin(), [&](const double d) { return func(d); });
        // Filtering all vectors by keep-flags.
        forEachVector(arg, [&](DataVectorRef& ref)
        {
            if (ref.values())
                keepByFlags(*ref.values(), keepFlags);
            else if (ref.strings())
                keepByFlags(*ref.strings(), keepFlags);
            else if (!ref.isNull())
            {
                DFG_ASSERT_IMPLEMENTED(false);
            }
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

    inline double ChartEntryOperation::axisStrToIndex(const StringView & sv)
    {
        if (sv == DFG_UTF8("x"))
            return axisIndex_x;
        else if (sv == DFG_UTF8("y"))
            return axisIndex_y;
        else
            return axisIndex_invalid;
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

        // Adds creator mapping, returns true if added, false otherwise (may happen e.g. if id is already present)
        bool add(StringViewUtf8 svId, CreatorFunc creator);

        // Convenience function for add operation from class that has id() and create() defined.
        template <class Operation_T>
        bool add();

        ChartEntryOperation createOperation(StringViewUtf8);

        DFG_MODULE_NS(cont)::MapVectorSoA<StringUtf8, CreatorFunc> m_knownOperations;
    };

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
        return add(typename Operation_T::id(), Operation_T::create);
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
        static ChartEntryOperation create(const CreationArgList& argList)
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

} // namespace operations

inline auto ChartEntryOperationManager::createOperation(StringViewUtf8 svFuncAndParams) -> ChartEntryOperation
{
    auto item = ParenthesisItem::fromStableView(svFuncAndParams);
    if (item.key().empty())
        return ChartEntryOperation();
    auto iter = m_knownOperations.find(item.key());
    if (iter != m_knownOperations.end())
        return iter->second(item);

    // Wasn't found from map, checking build-in operations.
    if (item.key() == operations::PassWindowOperation::id())
        return operations::PassWindowOperation::create(item);

    // No requested operation found.
    return ChartEntryOperation();
}


}} // module namespace
