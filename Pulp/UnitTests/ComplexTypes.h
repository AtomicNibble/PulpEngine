#pragma once

namespace testTypes
{
    // init it to diffrent vlaues to make sure resetConConters is called.
    extern int CONSRUCTION_COUNT;
    extern int DECONSRUCTION_COUNT;
    extern int MOVE_COUNT;

    void resetConConters(void);

    struct CustomType
    {
        CustomType() :
            var_(16)
        {
            CONSRUCTION_COUNT++;
        }
        CustomType(size_t val) :
            var_(val)
        {
            CONSRUCTION_COUNT++;
        }
        CustomType(const CustomType& oth) :
            var_(oth.var_)
        {
            ++CONSRUCTION_COUNT;
        }
        CustomType(CustomType&& oth) :
            var_(oth.var_)
        {
            ++CONSRUCTION_COUNT;
        }

        ~CustomType()
        {
            DECONSRUCTION_COUNT++;
        }

        CustomType& operator=(const CustomType& val)
        {
            var_ = val.var_;
            return *this;
        }
        CustomType& operator=(size_t val)
        {
            var_ = val;
            return *this;
        }

        bool operator==(const CustomType& oth) const
        {
            return var_ == oth.var_;
        }
        bool operator==(const size_t& oth) const
        {
            return var_ == oth;
        }

        bool operator<(const CustomType& rhs) const
        {
            return GetVar() < rhs.GetVar();
        }

        inline size_t GetVar(void) const
        {
            return var_;
        }

    private:
        size_t var_;

    public:
    };

    inline bool operator==(const CustomType& a, const size_t& b)
    {
        return a.GetVar() == b;
    }
    inline bool operator==(const size_t& a, const CustomType& b)
    {
        return a == b.GetVar();
    }

    struct CustomTypeComplex
    {
        CustomTypeComplex(size_t val, const char* pName) :
            var_(val),
            pName_(pName)
        {
            CONSRUCTION_COUNT++;
        }
        CustomTypeComplex(const CustomTypeComplex& oth) :
            var_(oth.var_),
            pName_(oth.pName_)
        {
            ++CONSRUCTION_COUNT;
        }
        CustomTypeComplex(CustomTypeComplex&& oth) :
            var_(oth.var_),
            pName_(oth.pName_)
        {
            ++MOVE_COUNT;
        }

        ~CustomTypeComplex()
        {
            DECONSRUCTION_COUNT++;
        }

        CustomTypeComplex& operator=(const CustomTypeComplex& val)
        {
            var_ = val.var_;
            return *this;
        }

        bool operator<(const CustomTypeComplex& rhs) const
        {
            return GetVar() < rhs.GetVar();
        }

        inline size_t GetVar(void) const
        {
            return var_;
        }
        inline const char* GetName(void) const
        {
            return pName_;
        }

    private:
        size_t var_;
        const char* pName_;
    };

} // namespace testTypes