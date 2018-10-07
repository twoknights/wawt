

                            //================
                            // class  WawtDump
                            //================

class  WawtDump : public Wawt::DrawProtocol {
  public:
    struct Indent {
        unsigned int d_indent;

        Indent& operator+=(unsigned int change) {
            d_indent += change;
            return *this;
        }

        Indent& operator-=(unsigned int change) {
            d_indent -= change;
            return *this;
        }

        Indent(unsigned int indent = 0) : d_indent(indent) { }
    };

    explicit WawtDump(std::ostream& os) : d_dumpOs(os) { }

    void     draw(const Wawt::DrawDirective&  widget,
                  const Wawt::String_t&       text)                override;

    void     getTextMetrics(Wawt::DrawDirective   *options,
                            Wawt::TextMetrics     *metrics,
                            const Wawt::String_t&  text,
                            double                 upperLimit = 0) override;

  private:
    Indent         d_indent;
    std::ostream&  d_dumpOs;
};
