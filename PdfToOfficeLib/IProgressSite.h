#pragma once

namespace HpdfToOffice
{
    class IProgressSite
    {
    public:
        IProgressSite() {}
        virtual ~IProgressSite() {}

    public:
        virtual void SetPercent(int percent) = 0;
    };
}