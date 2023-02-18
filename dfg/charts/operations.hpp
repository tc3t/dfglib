#pragma once

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
#include <regex>
#include "../dataAnalysis/smoothWithNeighbourAverages.hpp"
#include "../dataAnalysis/smoothWithNeighbourMedians.hpp"
#include "../math/FormulaParser.hpp"
#include "../cont/Flags.hpp"

#include "../str/format_fmt.hpp"

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
                return rawPtr() == nullptr;
            }

            const void*         rawPtr() const      { return (constValues() != nullptr) ? static_cast<const void*>(constValues()) : static_cast<const void*>(constStrings()); }

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

        ChartOperationPipeData(const ChartOperationPipeData& other);

        ChartOperationPipeData(ChartOperationPipeData&& other) noexcept;

        ChartOperationPipeData& operator=(const ChartOperationPipeData& other);

        ChartOperationPipeData& operator=(ChartOperationPipeData&& other) noexcept;

        template <class X_T, class ... Vals_T>
        ChartOperationPipeData(X_T&& xvals, Vals_T&& ... yvals);

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

        StringVector* editableStringsByIndex(size_t i);
        StringVector* editableStringsByRef(DataVectorRef& ref);

        // Sets active vectors.
        void setDataRefs(DataVectorRef ref0, DataVectorRef ref1 = DataVectorRef(), DataVectorRef ref2 = DataVectorRef());

        // Returns the number of active vectors
        size_t vectorCount() const;

        // Sets size of all active non-null vectors to zero.
        void setVectorSizesToZero();

     private:
         // Implements reference replacements for copying.
         template <class Cont_T>
         void replaceInternalReferences(Cont_T& myData, const std::vector<DataVectorRef>& otherRefs, const Cont_T& otherData);

    public:
        std::vector<DataVectorRef> m_vectorRefs;    // Stores references to data vectors and defines the actual vector count that this data has.
        // Note: using deque to avoid invalidation of resize.
        std::deque<ValueVectorD> m_valueVectors;    // Temporary value buffers that can be used e.g. if operation changes data type and can't edit existing.
        std::deque<StringVector> m_stringVectors;   // Temporary string buffers that can be used e.g. if operation changes data type and can't edit existing.
    };

    template <class X_T, class ... Vals_T>
    inline ChartOperationPipeData::ChartOperationPipeData(X_T&& xvals, Vals_T&& ... vals)
    {
        setDataRefs(xvals, std::forward<Vals_T>(vals)...);
    }

    inline ChartOperationPipeData::ChartOperationPipeData(const ChartOperationPipeData& other)
    {
        *this = other;
    }

    inline ChartOperationPipeData::ChartOperationPipeData(ChartOperationPipeData&& other) noexcept
    {
        *this = std::move(other);
    }

    inline auto ChartOperationPipeData::operator=(const ChartOperationPipeData& other) -> ChartOperationPipeData&
    {
        this->m_valueVectors = other.m_valueVectors;
        this->m_stringVectors = other.m_stringVectors;
        // Setting references taking care that if 'other' has reference to it's internal vectors,
        // 'this' refers to it's own internal vectors, not those of 'other'.
        this->m_vectorRefs = other.m_vectorRefs;
        replaceInternalReferences(this->m_valueVectors, other.m_vectorRefs, other.m_valueVectors);
        replaceInternalReferences(this->m_stringVectors, other.m_vectorRefs, other.m_stringVectors);
        return *this;
    }

    inline auto ChartOperationPipeData::operator=(ChartOperationPipeData&& other) noexcept -> ChartOperationPipeData&
    {
        // Note: std::vector move assignment is noexcept only since C++17.
        //       std::deque apparently does not have noexcept moving even in C++17 (https://gcc.gnu.org/legacy-ml/gcc-help/2017-08/msg00078.html)
        // While moves below may throw, considering such conditions to be corner cases where the resulting std::terminate()
        // that gets triggered due to this noexcept function throwing, is acceptable outcome.
        m_vectorRefs = std::move(other.m_vectorRefs);
        m_valueVectors = std::move(other.m_valueVectors);
        m_stringVectors = std::move(other.m_stringVectors);
        return *this;
    }

    template <class Cont_T>
    inline void ChartOperationPipeData::replaceInternalReferences(Cont_T& myData, const std::vector<DataVectorRef>& otherRefs, const Cont_T& otherData)
    {
        DFG_ASSERT_UB(myData.size() == otherData.size());
        DFG_ASSERT_UB(this->m_vectorRefs.size() == otherRefs.size());
        for (size_t i = 0, nCount = otherData.size(); i < nCount; ++i)
        {
            auto iter = std::find_if(otherRefs.begin(), otherRefs.end(), [&](const DataVectorRef& ref) { return ref.rawPtr() == &otherData[i]; });
            if (iter != otherRefs.end())
            {
                this->m_vectorRefs[iter - otherRefs.begin()] = DataVectorRef(&myData[i]);
            }
        }
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
        return editableStringsByIndex(static_cast<size_t>(&ref - &m_vectorRefs.front()));
    }

    inline auto ChartOperationPipeData::editableStringsByIndex(const size_t i) -> StringVector*
    {
        if (i >= m_valueVectors.max_size())
            return nullptr;
        if (!isValidIndex(m_stringVectors, i))
            m_stringVectors.resize(i + 1);
        return &m_stringVectors[i];
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
        using DefinitionArgStrList = std::vector<StringT>;
        static void defaultCall(ChartEntryOperation&, ChartOperationPipeData&);
        using OperationCall     = decltype(ChartEntryOperation::defaultCall);
        using DataVectorRef     = ChartOperationPipeData::DataVectorRef;

        // Error flags; set when errors are encountered.
        enum Error
        {
            error_missingInput                  = 0x1,    // Pipe data does not have expected data.
            error_noCallable                    = 0x2,    // Operation does not have callable.
            error_badCreationArgs               = 0x4,    // Arguments used to define operation are invalid.
            error_pipeDataVectorSizeMismatch    = 0x8,    // Input vectors have different size.
            error_unexpectedInputVectorCount    = 0x10,   // Input has unexpected vector count.
            error_unexpectedInputVectorTypes    = 0x20,   // Input has unexpected type (e.g. string instead of number)
            error_unableToCreateDataVectors     = 0x40,   // Operation failed because new data vectors couldn't be created.
            error_processingError               = 0x80,   // Error occured while operating, this can happen for example if creation args are bad and/or incompatible with input.
            error_unableToDefineVariables       = 0x100   // Failed to define variables to internal parser.
        };
        using ErrorMask = ::DFG_MODULE_NS(cont)::Flags<Error>;

        // Helper enum for storing creation arg string as numeric value.
        enum AxisIndex
        {
            axisIndex_invalid   = -1,
            axisIndex_x         = 0,
            axisIndex_y         = 1,
            axisIndex_z         = 2
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
        template <class Func_T, class PipeAccess_T>
        inline void privFilterBySingle(ChartOperationPipeData& arg, const double axis, Func_T&& keepPred, PipeAccess_T&& pipeAccess);

        // Convenience overload that filters using double vector
        template <class Func_T>
        inline void privFilterBySingle(ChartOperationPipeData& arg, const double axis, Func_T&& func);

        // Returns pointer to pipe vector by axis index.
        static ValueVectorD* privPipeVectorByAxisIndex(ChartOperationPipeData& arg, double index);

        // Returns ChartEntryOperation() from case that creation args were invalid.
        static ChartEntryOperation privInvalidCreationArgsResult();

        size_t argCount() const { return Max(m_argList.size(), m_argStrList.size()); }

        double       argAsDouble(const size_t nIndex) const;
        StringViewSz argAsString(const size_t nIndex) const;

        void storeArg(size_t nIndex, double);
        void storeArg(size_t nIndex, const StringView&);
        void storeArg(size_t nIndex, const StringViewSz& sv) { storeArg(nIndex, sv.toStringView()); }

        template <class Cont_T, class T>
        void privStoreArg(Cont_T& cont, size_t nIndex, T&& item);

        OperationCall*    m_pCall          = &ChartEntryOperation::defaultCall;
        ErrorMask         m_errors;
        DefinitionArgList    m_argList;
        DefinitionArgStrList m_argStrList; // TODO: unify arglist to single variant list.
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
        return m_errors.operator bool();
    }

    inline bool ChartEntryOperation::hasError(Error err) const
    {
        return (m_errors & err).operator bool();
    }

    inline double ChartEntryOperation::argAsDouble(const size_t nIndex) const
    {
        return isValidIndex(this->m_argList, nIndex) ? this->m_argList[nIndex] : std::numeric_limits<double>::quiet_NaN();
    }

    inline auto ChartEntryOperation::argAsString(const size_t nIndex) const -> StringViewSz
    {
        return isValidIndex(this->m_argStrList, nIndex) ? StringViewSz(this->m_argStrList[nIndex]) : StringViewSz(DFG_UTF8(""));
    }

    template <class Cont_T, class T>
    inline void ChartEntryOperation::privStoreArg(Cont_T& cont, const size_t nIndex, T&& item)
    {
        if (nIndex >= cont.max_size())
            return;
        if (!isValidIndex(cont, nIndex))
            cont.resize(nIndex + 1);
        cont[nIndex] = std::move(item);
    }

    inline void ChartEntryOperation::storeArg(const size_t nIndex, double val)
    {
        privStoreArg(m_argList, nIndex, std::move(val));
    }

    inline void ChartEntryOperation::storeArg(const size_t nIndex, const StringView& sv)
    {
        privStoreArg(m_argStrList, nIndex, sv.toString());
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

    template <class Func_T, class PipeAccess_T>
    inline void ChartEntryOperation::privFilterBySingle(ChartOperationPipeData& arg, const double axis, Func_T&& func, PipeAccess_T&& pipeAccess)
    {
        using namespace DFG_MODULE_NS(alg);
        auto pCont = pipeAccess(arg, axis);
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
        std::transform(pCont->begin(), pCont->end(), keepFlags.begin(), func);
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

    template <class Func_T>
    inline void ChartEntryOperation::privFilterBySingle(ChartOperationPipeData& arg, const double axis, Func_T&& func)
    {
        return privFilterBySingle(arg, axis, std::forward<Func_T>(func), [](ChartOperationPipeData& arg2, const double axis2) { return privPipeVectorByAxisIndex(arg2, axis2); });
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
        else if (sv == DFG_UTF8("z"))
            return axisIndex_z;
        else
            return axisIndex_invalid;
    }

    inline auto ChartEntryOperation::floatAxisValueToAxisIndex(double index) -> size_t
    {
        return (index >= 0 && index < 10000 && ::DFG_MODULE_NS(math)::isIntegerValued(index)) ? static_cast<size_t>(index) : (std::numeric_limits<size_t>::max)();
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
    namespace DFG_DETAIL_NS
    {
        // Common crtp-base for basic filter operations, derived class should have static filter() function that
        // takes three arguments (value, arg1, arg2) and should return true if 'value' should be kept, false if it should be removed.
        template <class Derived_T>
        class FilterOperation : public ChartEntryOperation
        {
        public:
            // Returns filter operation whose implementation is defined by Derived_T or invalid operation eg. if arguments are invalid.
            static ChartEntryOperation create(const CreateOperationArgs& argList);

            static void operation(ChartEntryOperation& op, ChartOperationPipeData& arg);
        }; // class FilterOperation

        template <class Derived_T>
        inline ChartEntryOperation FilterOperation<Derived_T>::create(const CreateOperationArgs& argList)
        {
            if (argList.valueCount() != 3)
                return privInvalidCreationArgsResult();
            const auto axis = axisStrToIndex(argList.value(0));
            if (axis == static_cast<int>(axisIndex_invalid))
                return privInvalidCreationArgsResult();
            ChartEntryOperation op(FilterOperation::operation);

            op.storeArg(0, axis);
            op.storeArg(1, argList.valueAs<double>(1));
            op.storeArg(2, argList.valueAs<double>(2));
            return op;
        }

        // Executes operation on pipe data.
        template <class Derived_T>
        inline void FilterOperation<Derived_T>::operation(ChartEntryOperation& op, ChartOperationPipeData& arg)
        {
            if (op.argCount() < 3)
            {
                op.setError(error_badCreationArgs);
                return;
            }
            const auto axis = op.argAsDouble(0);
            const auto arg1 = op.argAsDouble(1);
            const auto arg2 = op.argAsDouble(2);
            op.privFilterBySingle(arg, axis, [=](const double v) { return Derived_T::filter(v, arg1, arg2); });
        }
    } // namespace DFG_DETAIL_NS

    /** Implements pass window operation
     *      Filters out all data points (xi, yi, [zi]) where chosen coordinate (x/y/z) is not within [a, b]
     *  Id:
     *      passWindow
     *  Parameters:
     *      -0: axis, either x, y or z
     *      -1: pass window lower limit
     *      -2: pass window upper limit
     *  Dependencies
     *      -[x], [y] or [z] depending on parameter 0
     *  Outputs:
     *      -The same number of vectors as in input
     *  Details:
     *      -NaN handling: unspecified
     */
    class PassWindowOperation : public DFG_DETAIL_NS::FilterOperation<PassWindowOperation>
    {
    public:
        using BaseClass = DFG_DETAIL_NS::FilterOperation<PassWindowOperation>;

        // Returns operation id
        static SzPtrUtf8R id()
        {
            return DFG_UTF8("passWindow");
        }

        static bool filter(const double v, const double lowerBound, const double upperBound)
        {
            return v >= lowerBound && v <= upperBound;
        }
    }; // PassWindowOperation

    /** Implements block window operation
     *      Inverse of passWindow: filters out values in given window
     *  Id:
     *      blockWindow
     *  Details like for passWindow.
     */
    class BlockWindowOperation : public DFG_DETAIL_NS::FilterOperation<BlockWindowOperation>
    {
    public:
        using BaseClass = DFG_DETAIL_NS::FilterOperation<BlockWindowOperation>;

        // Returns operation id
        static SzPtrUtf8R id()
        {
            return DFG_UTF8("blockWindow");
        }

        // Executes operation on pipe data.
        static bool filter(const double v, const double lowerBound, const double upperBound)
        {
            return !PassWindowOperation::filter(v, lowerBound, upperBound);
        }
    }; // BlockWindowOperation

     /** Implements TextFilter operation that keeps text entries by match pattern.
     *  Id:
     *      textFilter
     *  Parameters:
     *      -0: axis, x, y or z
     *      -1: Match pattern, by default regular expression syntax
     *      -[2]: pattern type: default is reg_exp, currently it is the only one supported
     *      -[3]: negate: if 1, match will be negated, i.e. normally pattern "a" would keep texts having "a", with negate would keep texts not having "a"
     *  Dependencies
     *      -[x], [y] or [z] depending on parameter 0
     *  Outputs:
     *      -The same number of vectors as in input
     */
    class TextFilterOperation : public ChartEntryOperation
    {
    public:
        using BaseClass = ChartEntryOperation;

        static SzPtrUtf8R id();

        static ChartEntryOperation create(const CreationArgList& argList);

        static void operation(ChartEntryOperation& op, ChartOperationPipeData& arg);
    }; // TextFilterOperation

    inline auto TextFilterOperation::id() -> SzPtrUtf8R
    {
        return DFG_UTF8("textFilter");
    }

    inline auto TextFilterOperation::create(const CreationArgList& argList) -> ChartEntryOperation
    {
        if (argList.valueCount() < 2)
            return ChartEntryOperation();
        const auto axis = axisStrToIndex(argList.value(0));
        if (axis == static_cast<int>(axisIndex_invalid))
            return ChartEntryOperation();

        const auto svMatchPattern = argList.value(1);
        const auto svPatternType = argList.value(2);
        const auto svNegate = argList.value(3);
        const auto svCaseSensitivity = argList.value(4);

        // Checking if arguments have unsupported values
        if (   (!svPatternType.empty()     && svPatternType != DFG_UTF8("reg_exp"))
            || (!svNegate.empty() && (svNegate != DFG_UTF8("0") && svNegate != DFG_UTF8("1")))
            || (!svCaseSensitivity.empty() && svCaseSensitivity != DFG_UTF8("1"))
           )
        {
            return ChartEntryOperation();
        }
        
        // Testing that can create regex from given pattern
        try
        {
            std::regex(svMatchPattern.asUntypedView().toString());
        }
        catch (...)
        {
            return ChartEntryOperation();
        }

        ChartEntryOperation op(&TextFilterOperation::operation);

        op.storeArg(0, axis);
        op.storeArg(1, svMatchPattern);
        op.storeArg(2, svPatternType);
        op.storeArg(3, svNegate);
        return op;
    }

    // Executes operation on pipe data.
    inline void TextFilterOperation::operation(ChartEntryOperation& op, ChartOperationPipeData& arg)
    {
        if (op.argCount() < 2)
        {
            op.setError(error_badCreationArgs);
            return;
        }
        const auto axis = op.argAsDouble(0);
        const auto svMatchPattern = op.argAsString(1);
        const auto svNegate = (op.argAsString(3) == DFG_UTF8("1"));

        std::regex re;
        try
        {
            re.assign(svMatchPattern.asUntypedView().toString());
        }
        catch (...)
        {
            op.setError(error_badCreationArgs);
            return;
        }

        const auto pipeAccess = [](const ChartOperationPipeData& arg, const double axis) { return arg.constStringsByIndex(floatAxisValueToAxisIndex(axis)); };
        
        const auto filter = [&](const StringViewUtf8& sv)
        {
            const bool b = std::regex_match(sv.beginRaw(), sv.endRaw(), re);
            return (svNegate) ? !b : b;
        };
        op.privFilterBySingle(arg, axis, filter, pipeAccess);
    }

    /** Implements RegexFormatOperation that can be used to transform input strings
    *  Id:
    *      regexFormat
    *  Parameters:
    *      -0: axis, x, y or z
    *      -1: Regular expression
    *      -2: result format in fmt-syntax. {i} will be replaced by (i)'th capture of regexp. Maximum index is 10
    *           -Note that indexing is essentially 1-based, i.e. first parenthesis item has index 1, at index 0 is the full match like in https://en.cppreference.com/w/cpp/regex/match_results
    *  Dependencies
    *      -[x], [y] or [z] depending on parameter 0
    *  Outputs:
    *      -The same number of vectors as in input
    *      -Row count is not changed
    *  Errors (non-exhaustive list):
    *      -Invalid regex: error_badCreationArgs (only possible if RegexFormatOperation has been created directly, i.e. create() would return empty operation with bad regex)
    *      -Too many matches in regex: error_processingError
    *      -Invalid format string: no error is set, but all result strings will be empty
    *  Details:
    *      -Handling of encodings: unspecified, expected to have shortcomings e.g. if format string has some type of UTF8
    *           -https://stackoverflow.com/questions/60600550/force-utf-8-handling-for-stdstring-in-fmt
    *           -TODO: should have proper specification for this.
    *      -If regex doesn't match or format string is not valid, result will be an empty string.
    *  Examples
    *      -Basic example:
    *           -0: x
    *           -1: (\d)(\d).*
    *           -2: {2}{1}{0}
    *           -With input x = ["12", "34abc", "ab"], result is ["2112", "4334abc", ""]
    */
    class RegexFormatOperation : public ChartEntryOperation
    {
    public:
        using BaseClass = ChartEntryOperation;

        static SzPtrUtf8R id();

        static ChartEntryOperation create(const CreationArgList& argList);

        static void operation(ChartEntryOperation& op, ChartOperationPipeData& arg);
    }; // RegexFormat

    inline auto RegexFormatOperation::id() -> SzPtrUtf8R
    {
        return DFG_UTF8("regexFormat");
    }

    inline auto RegexFormatOperation::create(const CreationArgList& argList) -> ChartEntryOperation
    {
        if (argList.valueCount() != 3)
            return ChartEntryOperation();
        const auto axis = axisStrToIndex(argList.value(0));
        if (axis == static_cast<int>(axisIndex_invalid))
            return ChartEntryOperation();

        const auto svRegex = argList.value(1);
        const auto svFormat = argList.value(2);

        // Testing that can create regex from given pattern
        try
        {
            std::regex(svRegex.asUntypedView().toString());
        }
        catch (...)
        {
            return ChartEntryOperation();
        }

        ChartEntryOperation op(&RegexFormatOperation::operation);

        op.storeArg(0, axis);
        op.storeArg(1, svRegex);
        op.storeArg(2, svFormat);
        return op;
    }

    // Executes operation on pipe data.
    inline void RegexFormatOperation::operation(ChartEntryOperation& op, ChartOperationPipeData& arg)
    {
        if (op.argCount() != 3)
        {
            op.setError(error_badCreationArgs);
            return;
        }
        const auto axis = op.argAsDouble(0);
        const auto svRegex = op.argAsString(1);
        const auto svFormat = op.argAsString(2).asUntypedView();

        std::regex re;
        try
        {
            re.assign(svRegex.asUntypedView().toString()); // TODO: regex is already created in creation phase, should re-use that object.
        }
        catch (...)
        {
            op.setError(error_badCreationArgs);
            return;
        }

        auto pStrings = arg.stringsByIndex(floatAxisValueToAxisIndex(axis)); // Note: this automatically creates and returns an editable copy if input is const
        const auto toArg = [](const auto& match) { return StringViewC(match.first, match.second); };
        if (!pStrings) // No strings available
        {
            op.setError(error_missingInput);
            return;
        }
        std::string sResultTemp;
        for (auto& rStr : *pStrings)
        {
            std::cmatch bm;
            const auto bRegExMatch = std::regex_match(rStr.c_str().c_str(), bm, re);

            if (bRegExMatch)
            {
                const auto nMatchCount = bm.size();

                switch (nMatchCount)
                {
                    case  1: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0])); break;
                    case  2: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1])); break;
                    case  3: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2])); break;
                    case  4: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3])); break;
                    case  5: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4])); break;
                    case  6: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5])); break;
                    case  7: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6])); break;
                    case  8: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7])); break;
                    case  9: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7]), toArg(bm[8])); break;
                    case 10: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7]), toArg(bm[8]), toArg(bm[9])); break;
                    case 11: formatTo_fmt(sResultTemp, svFormat, toArg(bm[0]), toArg(bm[1]), toArg(bm[2]), toArg(bm[3]), toArg(bm[4]), toArg(bm[5]), toArg(bm[6]), toArg(bm[7]), toArg(bm[8]), toArg(bm[9]), toArg(bm[10])); break;
                    default: op.setError(error_processingError); break;
                }
            }
            rStr.rawStorage() = sResultTemp; // Note: no guarantees whether result preserves UTF8-encoding.
            sResultTemp.clear();
        }
    }

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
        op.storeArg(0, arg0);
        op.storeArg(1, arg1);
        return op;
    }

    // Executes operation on pipe data.
    inline void Smoothing_indexNb::operation(ChartEntryOperation& op, ChartOperationPipeData& arg)
    {
        size_t nNbRadius = 0;
        if (op.argCount() < 1 || !::DFG_MODULE_NS(math)::isFloatConvertibleTo(op.argAsDouble(0), &nNbRadius))
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
        const auto smoothingType = (op.argCount() >= 2) ? op.argAsDouble(1) : smoothingTypeAverage();
        if (smoothingType == smoothingTypeAverage())
            ::DFG_MODULE_NS(dataAnalysis)::smoothWithNeighbourAverages(*pY, nNbRadius);
        else if (smoothingType == smoothingTypeMedian())
            ::DFG_MODULE_NS(dataAnalysis)::smoothWithNeighbourMedians(*pY, nNbRadius);
        else
            op.setError(error_badCreationArgs);
    }


    /** Implements element-wise operation by given formula to data
    *  Id:
    *       formula
    *  Parameters:
    *       -[0]: axis to which value is formula is assigned to, either x or y.
    *       -[1]: formula. 
    *  Dependencies
    *       -Argument 0 defines one dependency, but formula definition may define others.
    *  Outputs:
    *       -Same data structure as inputs; element values may have changed only on axis specified by first argument.
    *  Details:
    *       -The following variables are available in formula (if they are available in input): x (value of x[i]), y (value of y[i])
    *  Examples:
    *       -formula("x", "y + 10"): for each element i, x[i] = y[i] + 10
    */
    class Formula : public ChartEntryOperation
    {
    public:
        static SzPtrUtf8R id();

        static ChartEntryOperation create(const CreationArgList& argList);

        static void operation(ChartEntryOperation& op, ChartOperationPipeData& arg);
    }; // class Formula

    inline auto Formula::id() -> SzPtrUtf8R
    {
        return DFG_UTF8("formula");
    }

    inline auto Formula::create(const CreationArgList& argList) -> ChartEntryOperation
    {
        if (argList.valueCount() < 2)
            return ChartEntryOperation();

        const auto axis = axisStrToIndex(argList.value(0));
        if (axis == static_cast<int>(axisIndex_invalid))
            return ChartEntryOperation();

        ChartEntryOperation op(&Formula::operation);

        op.storeArg(0, axis);;
        op.storeArg(1, argList.value(1));
        return op;
    }

    // Executes operation on pipe data.
    inline void Formula::operation(ChartEntryOperation& op, ChartOperationPipeData& arg)
    {
        if (op.argCount() < 2)
        {
            op.setError(error_badCreationArgs);
            return;
        }
        const auto axis = op.argAsDouble(0);
        const auto svFormula = op.argAsString(1);
        if (svFormula.empty())
            return;
 
        ::DFG_MODULE_NS(math)::FormulaParser parser;
        double x = std::numeric_limits<double>::quiet_NaN();
        double y = std::numeric_limits<double>::quiet_NaN();

        if (!parser.setFormula(svFormula))
        {
            op.setError(error_badCreationArgs);
            return;
        }

        const auto pValuesX = arg.constValuesByIndex(0);
        const auto pValuesY = arg.constValuesByIndex(1);

        if (!pValuesX && !pValuesY)
        {
            op.setError(error_missingInput);
            return;
        }
            
        auto pOutput = op.privPipeVectorByAxisIndex(arg, axis);

        if (!pOutput)
        {
            op.setError(error_unableToCreateDataVectors);
            return;
        }

        if ((pValuesX && !parser.defineVariable("x", &x)) || (pValuesY && !parser.defineVariable("y", &y)))
        {
            op.setError(error_unableToDefineVariables);
            return;
        }

        const auto nSize = (pValuesX) ? pValuesX->size() : pValuesY->size();
        if (pValuesX && pValuesY && pValuesX->size() != pValuesY->size())
        {
            op.setError(error_pipeDataVectorSizeMismatch);
            return;
        }
        if (pOutput->size() != nSize)
        {
            op.setError(error_pipeDataVectorSizeMismatch);
            return;
        }
        ::DFG_MODULE_NS(math)::FormulaParser::ReturnStatus evalStatus;
        size_t nEvalErrorCount = 0;
        bool bParseError = false;
        for (size_t i = 0; i < nSize; ++i)
        {
            double val;
            if (!bParseError)
            {
                if (pValuesX)
                    x = (*pValuesX)[i];
                if (pValuesY)
                    y = (*pValuesY)[i];
                val = parser.evaluateFormulaAsDouble(&evalStatus);
                if (!evalStatus)
                {
                    // Evaluation failed. Checking if it was because of certain type of syntax error where it's not reasonable to try to re-evaluate for every element.
                    ++nEvalErrorCount;
                    bParseError = (parser.backendType() == 1 && evalStatus.errorCode() == 0); // backendType() == 1 == muparser, 0 == ecUNEXPECTED_OPERATOR.
                }
            }
            else
            {
                val = std::numeric_limits<double>::quiet_NaN();
                ++nEvalErrorCount;
            }
            (*pOutput)[i] = val;
        }
        if (nEvalErrorCount > 0)
            op.setError(error_processingError);
    }

} // namespace operations

inline ChartEntryOperationManager::ChartEntryOperationManager()
{
    // Adding built-in operations.
    add<operations::PassWindowOperation>();
    add<operations::BlockWindowOperation>();
    add<operations::Smoothing_indexNb>();
    add<operations::Formula>();
    add<operations::TextFilterOperation>();
    add<operations::RegexFormatOperation>();
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
